#include "core/trade_event.hpp"
#include "fixed_point.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "readerwriterqueue.h"
#include "signals/signals.hpp"
#include "ui/dashboard_types.hpp"
#include "ui/root.hpp"
#include <iostream>
#include <thread>

#include "line_view.hpp"
#include "mmfile.hpp"
#include "parser/csv.hpp"
#include "parser/nasdaq.hpp"
#include "render.hpp"
#include <chrono>

struct NasdaqFile {
  using symbol_t = std::string;
  using order_id_t = uint64_t;

  static void process_file(MappedFile &file, const auto &on_parsed) {
    size_t offset = 0;
    std::span<const uint8_t> data{file.data(), file.size()};
    while (offset < data.size()) {
      if (auto msg = parser::nasdaq::parse_message(data, offset))
        on_parsed(*msg);
      else
        std::cout << fmt::LineTag::Error("ERROR") << msg.error() << nl;
    }
  }

  static void update_book(auto &book, auto &msg) {
    parser::nasdaq::processNasdaqMessage(book, msg);
  }
};

struct CSVFile {
  using symbol_t = decltype(parser::csv::parse_line("")->symbol);
  using order_id_t = FixedSizeOrderID;

  static void process_file(MappedFile &file, const auto &on_parsed) {
    LineView lines(reinterpret_cast<const char *>(file.data()), file.size());
    for (const auto &line : lines) {
      if (auto msg = parser::csv::parse_line(line))
        on_parsed(*msg);
      else
        std::cout << "ERROR " << msg.error() << nl;
    }
  }

  static void update_book(auto &book, auto &msg) {
    parser::csv::process_csv_message(book, msg);
  }
};

using FileSource = CSVFile;
// using FileSource = NasdaqFile;

using Traits = OrderBookTraits<FileSource::order_id_t, FixedPoint<4>, uint32_t>;

void update_level(const auto &price, const auto &quantity, auto &levels) {
  auto it = std::lower_bound(levels.begin(), levels.end(), price,
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

struct FullySequential {
  template <typename Iterator>
  static void apply(ui::OrderBookSnapshot &snapshot, const Iterator begin,
                    const Iterator end) {
    for (auto current = begin; current != end; ++current)
      update_level(current->price, current->quantity,
                   current->side == side_t::ASK ? snapshot.l2_asks
                                                : snapshot.l2_bids);
  }
};

struct SideSequential {
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
      update_level(current->price, current->quantity, levels);
  }
};

struct MergeEvents {
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

      update_level(current->price, current->quantity, levels);

      ++current;
    }
    --current;
    update_level(current->price, current->quantity, levels);
  }
};

template <typename Strategy> struct UIControllerBase {
  using Event = LevelQuantityEvent<Traits>;
  using UIQueue = moodycamel::ReaderWriterQueue<LevelQuantityEvent<Traits>>;
  UIQueue m_ui_queue{1024};

  UIControllerBase() { workload.reserve(1024); }

  void process_update_queue(ui::OrderBookSnapshot &snapshot) {
    Event event;
    while (workload.size() < 1024 && m_ui_queue.try_dequeue(event))
      workload.emplace_back(std::move(event));

    snapshot.symbol = "NVDA";

    Strategy::apply(snapshot, workload.begin(), workload.end());
    workload.clear();
  }

private:
  std::vector<Event> workload;
};

// using UIController = UIControllerBase<FullySequential>;
// using UIController = UIControllerBase<SideSequential>;
using UIController = UIControllerBase<MergeEvents>;
UIController ui_controller;

template <typename Traits> struct EventHandler {
  void update(const TradeEvent<Traits> &event) { (void)event; }
  void update(const OrderMatchedEvent<Traits> &event) { (void)event; }
  void update(const LevelQuantityEvent<Traits> &event) {
    bool added = ui_controller.m_ui_queue.enqueue(event);
    (void)added;
    assert(added);
  }
};

using SignalAgregator = signals::StaticComposite<Traits, EventHandler<Traits>>;
using Book = OrderBook<Traits, SignalAgregator>;

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "../docs/given_example.csv";

  MappedFile file(filename);
  if (!file.data()) {
    std::cerr << "Failed to open or map file: " << filename << nl;
    return 1;
  }

  Book book;

  auto ui_thread = std::jthread([&]() {
    ui::Root root;
    auto callback =
        std::bind_front(&UIController::process_update_queue, &ui_controller);
    root.update_queue = callback;
    root.run();
  });

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(4s);

  FileSource::process_file(file, [&](const auto &msg) {
    std::cout << fmt::LineTag::Message("VALID") << msg << nl;
    FileSource::update_book(book, msg);
    // render_horizontal_orderbook(book);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    // std::cin.get();
  });

  return 0;
}
