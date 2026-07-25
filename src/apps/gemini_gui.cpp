#include "book/l2_adapter.hpp"
#include "core/trade_event.hpp"
#include "fixed_point.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "parser/gemini.hpp"
#include "platform/websocket.hpp"
#include "ui/dashboard_types.hpp"
#include "ui/message_queue.hpp"
#include "ui/root.hpp"

#include <iostream>
#include <thread>

using Traits = OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, FixedPoint<4>>;
using Traits2 = OrderBookTraitsL2<Traits>::Traits;
using Event = LevelQuantityEvent<Traits2>;

// using Strategy = ui::FullySequential<ui::GeminiLevelUpdate>;
// using Strategy = ui::SideSequential<ui::GeminiLevelUpdate>;
using Strategy = ui::MergeEvents<ui::GeminiLevelUpdate>;

using MessageQueue = ui::MessageQueue<Event, Strategy>;
MessageQueue msg_queue;

template <typename Traits> struct EventHandler {
  void update(const TradeEvent<Traits> &event) { (void)event; }
  void update(const OrderMatchedEvent<Traits> &event) { (void)event; }
  void update(const LevelQuantityEvent<Traits> &event) {
    msg_queue.push(event);
  }
};

using Book = OrderBookL2Adapter<Traits, EventHandler<Traits2>>;

using symbol_t = std::string;
std::unordered_map<symbol_t, Book> ticker_books;

struct Gemini {
  using symbol_t = std::string;
  using File = std::pair<symbol_t, std::string>;

  static std::optional<File> open(const std::string &ticker) {
    const std::string uri = "wss://api.gemini.com/v1/marketdata/" + ticker;
    return {std::make_pair(ticker, uri)};
  }

  static symbol_t symbol(const auto msg) {
    (void)msg;
    return "BTCUSD";
    /*
const auto &[file, parsed_message] = msg;
const auto &[ticker, uri] = file;
return uri;
    */
  }

  static void update_book(auto &book, auto &parsed_msg) {
    bool success = book.setPriceLevel(parsed_msg.side, parsed_msg.price,
                                      parsed_msg.quantity);
    (void)success;

#define DEBUG 1
#ifdef DEBUG
    if (book.isCrossedOrderBook())
      std::cout << "ERROR: Crossed order book" << std::endl;
#endif
  }

  static void wibble(const std::string &msg) {
    gemini::parse(msg, [](const auto &parsed_msg) {
      auto [it, added] = ticker_books.try_emplace(Gemini::symbol(parsed_msg));
      auto &book = it->second;

      Gemini::update_book(book, parsed_msg);
    });
  }

  static void process_file(auto &file, const auto &on_parsed) {
    const auto &[ticker, uri] = *file;
    ws::connect(uri, on_parsed);
  }
};

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "BTCUSD";

  auto ui_thread = std::jthread([&]() {
    auto file = Gemini::open(filename);
    if (!file) {
      std::cerr << "Failed to open or map file: " << filename << nl;
      return;
    }

    Gemini::process_file(file, Gemini::wibble);
  });

  ui::Root root;
  auto callback =
      std::bind_front(&MessageQueue::process_update_queue, &msg_queue);
  root.update_queue = callback;
  root.run();

  return 0;
}
