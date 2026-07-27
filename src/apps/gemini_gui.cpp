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

using TraitsL3 =
    OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, FixedPoint<4>>;
using Traits = OrderBookTraitsL2<TraitsL3>::Traits;
using Event = LevelQuantityEvent<Traits>;

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

using Book = OrderBookL2Adapter<Traits, EventHandler<Traits>>;
Book book;

static void update_book(auto &book, auto &msg) {
  bool success = book.setPriceLevel(msg.side, msg.price, msg.quantity);
  (void)success;

#define DEBUG 1
#ifdef DEBUG
  if (book.isCrossedOrderBook())
    std::cout << "ERROR: Crossed order book" << std::endl;
#endif
}

static void on_ws_message(const std::string &text) {
  gemini::parse(text, [](const auto &msg) { update_book(book, msg); });
}

int main(int argc, char *argv[]) {
  const std::string ticker = argc > 1 ? argv[1] : "BTCUSD";

  auto ui_thread = std::jthread([&]() {
    const std::string uri = "wss://api.gemini.com/v1/marketdata/" + ticker;
    ws::connect(uri, on_ws_message);
  });

  ui::Root root;
  auto callback =
      std::bind_front(&MessageQueue::process_update_queue, &msg_queue);
  root.update_queue = callback;
  root.run();

  return 0;
}
