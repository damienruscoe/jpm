#include "parser/gemini.hpp"
#include "book/l2_adapter.hpp"
#include "core/trade_event.hpp"
#include "fixed_point.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "platform/websocket.hpp"
#include "render.hpp"
#include "str_utils.hpp"

#include <iostream>

using TraitsL3 =
    OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, FixedPoint<4>>;
using Traits = OrderBookTraitsL2<TraitsL3>::Traits;

using Book = OrderBookL2Adapter<Traits>;
Book book;

static void update_book(auto &book, auto &msg) {
  bool success = book.setPriceLevel(msg.side, msg.price, msg.quantity);
  (void)success;

#define DEBUG 1
#ifdef DEBUG
  if (book.isCrossedOrderBook())
    std::cout << fmt::LineTag::Error("ERROR") << "Crossed order book"
              << std::endl;
#endif
}

static void on_ws_message(const std::string &text) {
  gemini::parse(text,
                [](const auto &parsed_msg) { update_book(book, parsed_msg); });
  render_horizontal_orderbook(book);
}

int main(int argc, char *argv[]) {
  std::string ticker = argc > 1 ? argv[1] : "BTCUSD";
  const std::string uri = "wss://api.gemini.com/v1/marketdata/" + ticker;

  ws::connect(uri, on_ws_message);
}
