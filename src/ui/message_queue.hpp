#pragma once

#include "core/trade_event.hpp"
#include "readerwriterqueue.h"
#include "ui/dashboard_types.hpp"

namespace ui {

struct DefaultLevelUpdate {
  static void update_level(const auto &evt, auto &levels) {
    auto it =
        std::lower_bound(levels.begin(), levels.end(), evt.price,
                         [](const auto &level, const auto &price) {
                           return level.price < static_cast<double>(price);
                         });

    const bool new_level =
        it == levels.end() || it->price != static_cast<double>(evt.price);

    if (new_level) {
      if (evt.quantity != 0)
        levels.insert(it, {static_cast<double>(evt.price),
                           evt.quantity,
                           evt.order_count,
                           {}});
    } else if (evt.quantity == 0)
      levels.erase(it);
    else
      it->size = evt.quantity;
  }
};

struct GeminiLevelUpdate {
  static void update_level(const auto &evt, auto &levels) {
    auto it =
        std::lower_bound(levels.begin(), levels.end(), evt.price,
                         [](const auto &level, const auto &price) {
                           return level.price < static_cast<double>(price);
                         });

    const bool new_level =
        it == levels.end() || it->price != static_cast<double>(evt.price);

    const double scalar = 100000.0;
    if (new_level) {
      if (evt.quantity != 0)
        levels.insert(it,
                      {static_cast<double>(evt.price),
                       (uint64_t)(scalar * static_cast<double>(evt.quantity)),
                       1,
                       {}});
    } else if (evt.quantity == 0)
      levels.erase(it);
    else
      it->size = (scalar * static_cast<double>(evt.quantity));
  }
};

template <typename LevelUpdate> struct FullySequential {
  template <typename Iterator>
  static void apply(ui::OrderBookSnapshot &snapshot, const Iterator begin,
                    const Iterator end) {
    for (auto current = begin; current != end; ++current)
      LevelUpdate::update_level(*current, current->side == side_t::ASK
                                              ? snapshot.l2_asks
                                              : snapshot.l2_bids);
  }
};

template <typename LevelUpdate> struct SideSequential {
  template <typename Iterator>
  static void apply(ui::OrderBookSnapshot &snapshot, const Iterator begin,
                    const Iterator end) {
    const auto asks = begin;
    const auto bids = std::stable_partition(
        asks, end, [](const auto &e) { return e.side == side_t::ASK; });

    process_side(snapshot.l2_asks, asks, bids);
    process_side(snapshot.l2_bids, bids, end);
  }

private:
  static void process_side(auto &levels, const auto begin, const auto end) {
    for (auto current = begin; current != end; ++current)
      LevelUpdate::update_level(*current, levels);
  }
};

template <typename LevelUpdate> struct MergeEvents {
  template <typename Iterator>
  static void apply(ui::OrderBookSnapshot &snapshot, const Iterator begin,
                    const Iterator end) {
    const auto asks = begin;
    const auto bids = std::stable_partition(
        asks, end, [](const auto &e) { return e.side == side_t::ASK; });

    process_side(snapshot.l2_asks, asks, bids);
    process_side(snapshot.l2_bids, bids, end);
  }

private:
  static void process_side(auto &levels, const auto begin, const auto end) {
    if (begin == end)
      return;

    std::stable_sort(begin, end, [](const auto &e1, const auto &e2) {
      return e1.price < e2.price;
    });

    auto current = begin;
    while (true) {
      current =
          std::adjacent_find(current, end, [](const auto &e1, const auto &e2) {
            return e1.price != e2.price;
          });
      if (current == end)
        break;

      LevelUpdate::update_level(*current, levels);

      ++current;
    }
    --current;
    LevelUpdate::update_level(*current, levels);
  }
};

template <typename Event, typename Strategy> struct MessageQueue {
  MessageQueue() { workload.reserve(1024); }

  void push(const Event &event) {
    bool added = m_ui_queue.enqueue(event);
    (void)added;
    assert(added);
  }

  void process_update_queue(ui::OrderBookSnapshot &snapshot) {
    Event event;
    while (workload.size() < 1024 && m_ui_queue.try_dequeue(event))
      workload.emplace_back(std::move(event));

    snapshot.symbol = "NVDA";

    Strategy::apply(snapshot, workload.begin(), workload.end());
    workload.clear();
  }

private:
  using UIQueue = moodycamel::ReaderWriterQueue<Event>;

  UIQueue m_ui_queue{1024};
  std::vector<Event> workload;
};

} // namespace ui
