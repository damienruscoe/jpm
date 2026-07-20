#include "core/trade_event.hpp"
#include "order_book.hpp"
#include "parser/parsers.hpp"
#include "signals/signals.hpp"
#include "ui/message_queue.hpp"
#include "ui/root.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

// using FileSource = CSVFile;
using FileSource = DatabentoFile;
// using FileSource = NasdaqFile;

using Traits =
    OrderBookTraits<FileSource::order_id_t, FileSource::Price, uint32_t>;

// using Strategy = ui::FullySequential;
// using Strategy = ui::SideSequential;
using Strategy = ui::MergeEvents;

using UIController = ui::UIControllerBase<LevelQuantityEvent<Traits>, Strategy>;
UIController ui_controller;

template <typename Traits> struct EventHandler {
  void update(const TradeEvent<Traits> &event) { (void)event; }
  void update(const OrderMatchedEvent<Traits> &event) { (void)event; }
  void update(const LevelQuantityEvent<Traits> &event) {
    ui_controller.push(event);
    std::this_thread::sleep_for(10ms);
  }
};

using Book = OrderBook<Traits, EventHandler<Traits>>;

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "../docs/given_example.csv";

  auto file = FileSource::open(filename);
  if (!file) {
    std::cerr << "Failed to open or map file: " << filename << nl;
    return 1;
  }

  auto ui_thread = std::jthread([&]() {
    ui::Root root;
    auto callback =
        std::bind_front(&UIController::process_update_queue, &ui_controller);
    root.update_queue = callback;
    root.run();
  });

  std::this_thread::sleep_for(4s);

  std::unordered_map<FileSource::symbol_t, Book> ticker_books;
  FileSource::process_file(file, [&](const auto &msg) {
    auto [it, added] = ticker_books.try_emplace(FileSource::symbol(msg));
    auto &book = it->second;

    FileSource::update_book(book, msg);
  });
}
