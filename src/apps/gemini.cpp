#include "parser/gemini.hpp"
#include "book/l2_adapter.hpp"
#include "fixed_point.hpp"
#include "order_book.hpp"
#include "platform/websocket.hpp"
#include "render.hpp"
#include "str_utils.hpp"

#include <iostream>

using Traits = OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, FixedPoint<4>>;
using Book = OrderBookL2Adapter<Traits>;

struct Venue {
  using symbol_t = std::string;
};

std::unordered_map<Venue::symbol_t, Book> ticker_books;

struct Gemini {
  using symbol_t = std::string;

  static symbol_t symbol(const auto msg) {
    (void)msg;
    return "BTCUSD";
    /*
const auto &[file, parsed_message] = msg;
const auto &[market_str, uri] = file;
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
      std::cout << fmt::LineTag::Error("ERROR") << "Crossed order book"
                << std::endl;
#endif
  }

  static void wibble(const std::string &msg) {
    gemini::parse(msg, [](const auto &parsed_msg) {
      auto [it, added] = ticker_books.try_emplace(Gemini::symbol(parsed_msg));
      auto &book = it->second;

      Gemini::update_book(book, parsed_msg);
      render_horizontal_orderbook(book);
    });
  }
};

int main(int argc, char *argv[]) {
  std::string market_str = argc > 1 ? argv[1] : "BTCUSD";
  const std::string uri = "wss://api.gemini.com/v1/marketdata/" + market_str;

  ws::connect(uri, Gemini::wibble);
}
