#pragma once

#include "expected.hpp"
#include "fixed_point.hpp"

#include <iostream>
#include <string_view>

namespace parser {

namespace csv {

enum class RequestType {
  New,
  Cancel,
  Amend,
  AmendPriceQuantity,
  AmendQuantity,
  Ignored
};
enum class Side { Buy, Sell };

struct Message {
  uint32_t symbol;
  RequestType type;
  std::string_view order_id;
  Side side;
  uint32_t quantity;
  FixedPoint<4> price;
};

std::ostream &operator<<(std::ostream &os, const Message &msg);

Expected<Message, std::string> parse_line(std::string_view line);

template <typename Book>
void process_csv_message(Book &book, const Message &msg) {
  switch (msg.type) {
  case RequestType::New: {
    const auto side = msg.side == Side::Buy ? Book::Side::BID : Book::Side::ASK;
    auto added = book.newOrder(msg.order_id, side, msg.price, msg.quantity);
    if (!added)
      std::cout << "ERROR " << "Adding new order failed" << nl;
    break;
  }
  case RequestType::Cancel: {
    auto cancelled = book.cancel(msg.order_id);
    if (!cancelled)
      std::cout << "ERROR " << "Cancelling existing order failed" << nl;
    break;
  }
  case RequestType::AmendPriceQuantity: {
    auto amended = book.amend(msg.order_id, msg.price, msg.quantity);
    if (!amended)
      std::cout << "ERROR " << "Amending order failed" << nl;
    break;
  }
  default:
    break;
  }
}

} // namespace csv

} // namespace parser
