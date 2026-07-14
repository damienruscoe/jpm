#if 0

// Benchmark for the three UIController::process_update_queue strategies.
//
// The original code selects a strategy at compile time via #ifdef
// (FULLY_SEQUENTIAL / SIDE_SEQUENTIAL / merged-default). For a fair
// comparison we want all three to run against *identical* generated
// batches in the same process, so this harness swaps the #ifdef for a
// compile-time Strategy enum + `if constexpr`, but the three code paths
// (process_sequentially / process_sides_sequentially / process_merged_events)
// and update_level are otherwise verbatim from what you posted.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "readerwriterqueue.h"

// ---------------------------------------------------------------------
// Stand-in domain types (not shown in your snippet, reconstructed to be
// representative: integer tick price + uint32 quantity, matches the
// static_cast<double>(price) in update_level).
// ---------------------------------------------------------------------
struct Traits {
  using price_t = std::int64_t;
  using quantity_t = std::uint32_t;
};

enum class side_t { BID, ASK };

template <typename TraitsT>
struct LevelQuantityEvent {
  typename TraitsT::price_t price;
  typename TraitsT::quantity_t quantity;
  side_t side;
};

namespace ui {
struct Level {
  double price;
  std::uint32_t size;
  std::uint32_t order_count;
  std::array<std::uint64_t, 2> reserved{};
};
struct OrderBookSnapshot {
  std::string symbol;
  std::vector<Level> l2_bids;
  std::vector<Level> l2_asks;
};
}  // namespace ui

enum class Strategy { FullySequential, SideSequential, Merged };

template <typename Traits, Strategy S>
struct UIController {
  using Event = LevelQuantityEvent<Traits>;
  using UIQueue = moodycamel::ReaderWriterQueue<Event>;
  UIQueue m_ui_queue{1024};

  void process_update_queue(ui::OrderBookSnapshot &snapshot) {
    size_t count{0};
    Event event;
    while (count < workload.size() && m_ui_queue.try_dequeue(event))
      workload[count++] = std::move(event);
    snapshot.symbol = "NVDA";
    if constexpr (S == Strategy::FullySequential)
      process_sequentially(snapshot, count);
    else if constexpr (S == Strategy::SideSequential)
      process_sides_sequentially(snapshot, count);
    else
      process_merged_events(snapshot, count);
  }

  std::array<Event, 1024> workload;

 private:
  void update_level(const auto &price, const auto &quantity, auto &levels) {
    auto it =
        std::lower_bound(levels.begin(), levels.end(), price,
                          [](const auto &level, const auto &price) {
                            return level.price < static_cast<double>(price);
                          });
    const bool new_level =
        it == levels.end() || it->price != static_cast<double>(price);
    if (new_level) {
      if (quantity != 0)
        levels.insert(it, {static_cast<double>(price), quantity, 0, {}});
    } else if (quantity == 0)
      levels.erase(it);
    else
      it->size = quantity;
  }

  void process_side_sequentially(auto begin, const auto end, auto &levels) {
    for (; begin != end; ++begin)
      update_level(begin->price, begin->quantity, levels);
  }

  void process_side(auto begin, const auto end, auto &levels) {
    if (begin == end) return;
    std::stable_sort(begin, end, [](const Event &e1, const Event &e2) {
      return e1.price < e2.price;
    });
    while (true) {
      begin = std::adjacent_find(
          begin, end, [](const Event &e1, const Event &e2) {
            return e1.price != e2.price;
          });
      if (begin == end) break;
      update_level(begin->price, begin->quantity, levels);
      ++begin;
    }
    --begin;
    update_level(begin->price, begin->quantity, levels);
  }

  void process_sequentially(ui::OrderBookSnapshot &snapshot, size_t count) {
    auto current = workload.begin();
    const auto end = current + count;
    for (; current != end; ++current)
      update_level(current->price, current->quantity,
                    current->side == side_t::ASK ? snapshot.l2_asks
                                                  : snapshot.l2_bids);
  }

  void process_sides_sequentially(ui::OrderBookSnapshot &snapshot,
                                   size_t count) {
    const auto asks = workload.begin();
    const auto end = asks + count;
    const auto bids = std::stable_partition(
        asks, end, [](const Event &e) { return e.side == side_t::ASK; });
    process_side_sequentially(asks, bids, snapshot.l2_asks);
    process_side_sequentially(bids, end, snapshot.l2_bids);
  }

  void process_merged_events(ui::OrderBookSnapshot &snapshot, size_t count) {
    const auto asks = workload.begin();
    const auto end = asks + count;
    const auto bids = std::stable_partition(
        asks, end, [](const Event &e) { return e.side == side_t::ASK; });
    process_side(asks, bids, snapshot.l2_asks);
    process_side(bids, end, snapshot.l2_bids);
  }
};

// ---------------------------------------------------------------------
// Workload generation
// ---------------------------------------------------------------------
using Event = LevelQuantityEvent<Traits>;

// Generates `batch_size` events. `dup_factor` controls how many events on
// average target the same price (simulates a book that's getting hammered
// at a few price levels vs one where every event is a fresh price).
std::vector<Event> make_batch(std::mt19937_64 &rng, size_t batch_size,
                               int dup_factor, std::int64_t price_lo,
                               std::int64_t price_hi) {
  size_t unique_prices = std::max<size_t>(1, batch_size / dup_factor);
  std::uniform_int_distribution<std::int64_t> price_dist(price_lo, price_hi);
  std::vector<std::int64_t> prices(unique_prices);
  for (auto &p : prices) p = price_dist(rng);

  std::uniform_int_distribution<size_t> pick(0, unique_prices - 1);
  std::uniform_int_distribution<std::uint32_t> qty_dist(1, 5000);
  std::bernoulli_distribution side_dist(0.5);

  std::vector<Event> batch(batch_size);
  for (auto &e : batch) {
    e.price = prices[pick(rng)];
    e.quantity = qty_dist(rng);
    e.side = side_dist(rng) ? side_t::ASK : side_t::BID;
  }
  return batch;
}

template <Strategy S>
double run_scenario(const char *label, size_t batch_size, int dup_factor,
                     int warm_levels, int iterations) {
  std::mt19937_64 rng(42);
  UIController<Traits, S> controller;
  ui::OrderBookSnapshot snapshot;

  // Fixed, bounded price universe (~2*warm_levels ticks) so the book
  // reaches a realistic steady-state depth instead of growing unboundedly.
  // This is the important fix over my first pass: with a wide-open price
  // range every "new" random price is a fresh insert into an
  // ever-growing vector, which drowns out the strategy differences under
  // O(n) memmove noise unrelated to what we're actually comparing.
  constexpr std::int64_t price_lo = 100'000;
  const std::int64_t price_hi = price_lo + warm_levels * 2;

  // Pre-warm the book so steady-state updates mostly hit existing levels
  // rather than paying insert cost every time.
  auto warm = make_batch(rng, warm_levels * 4, 1, price_lo, price_hi);
  for (auto &e : warm) controller.m_ui_queue.try_enqueue(e);
  controller.process_update_queue(snapshot);

  std::vector<double> samples;
  samples.reserve(iterations);

  for (int i = 0; i < iterations; ++i) {
    auto batch = make_batch(rng, batch_size, dup_factor, price_lo, price_hi);
    for (auto &e : batch) controller.m_ui_queue.try_enqueue(e);

    auto t0 = std::chrono::steady_clock::now();
    controller.process_update_queue(snapshot);
    auto t1 = std::chrono::steady_clock::now();

    samples.push_back(
        std::chrono::duration<double, std::nano>(t1 - t0).count());
  }

  std::sort(samples.begin(), samples.end());
  double mean = std::accumulate(samples.begin(), samples.end(), 0.0) /
                samples.size();
  double median = samples[samples.size() / 2];
  double p95 = samples[static_cast<size_t>(samples.size() * 0.95)];

  std::printf("%-18s batch=%4zu dup=%2dx  mean=%7.0fns  median=%7.0fns  "
              "p95=%7.0fns  ns/event(mean)=%6.1f  book_depth(bid/ask)=%zu/%zu\n",
              label, batch_size, dup_factor, mean, median, p95,
              mean / batch_size, snapshot.l2_bids.size(),
              snapshot.l2_asks.size());
  return mean;
}

int main() {
  constexpr int iterations = 3000;
  struct Scenario {
    size_t batch_size;
    int dup_factor;
    int warm_levels;  // pre-populated levels per side -> book depth
  };
  std::vector<Scenario> scenarios = {
      // shallow book (~130 levels/side, typical L2 panel depth)
      {1024, 1, 64},
      {1024, 4, 64},
      {1024, 16, 64},
      {256, 1, 64},
      {256, 16, 64},
      // deep book (~1000 levels/side) - stresses update_level's O(depth)
      // insert/erase, which is where Merged's fewer calls should matter.
      {1024, 1, 500},
      {1024, 4, 500},
      {1024, 16, 500},
      // very deep book (~4000 levels/side)
      {1024, 4, 2000},
      {1024, 16, 2000},
  };

  for (auto &sc : scenarios) {
    std::printf("--- batch=%zu dup=%dx book_depth~%d ---\n", sc.batch_size,
                sc.dup_factor, sc.warm_levels);
    double seq = run_scenario<Strategy::FullySequential>(
        "FullySequential", sc.batch_size, sc.dup_factor, sc.warm_levels,
        iterations);
    double side = run_scenario<Strategy::SideSequential>(
        "SideSequential", sc.batch_size, sc.dup_factor, sc.warm_levels,
        iterations);
    double merged = run_scenario<Strategy::Merged>(
        "Merged", sc.batch_size, sc.dup_factor, sc.warm_levels, iterations);
    std::printf("  speedup Merged vs FullySequential: %.2fx | vs "
                "SideSequential: %.2fx\n\n",
                seq / merged, side / merged);
  }
  return 0;
}

#endif
