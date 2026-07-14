#pragma once
#include "csv.hpp" // reused for Side{Buy,Sell} only — no edits made to parser.hpp/.cpp
#include "expected.hpp"
#include "fixed_point.hpp"
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

namespace parser {

namespace nasdaq {

enum class NasdaqMessageType {
  StockDirectory,
  MarketMaker,
  OrderAdd,
  OrderAddWithMPID,
  OrderExecute,
  OrderExecuteWithPrice,
  OrderCancel,
  OrderDelete,
  OrderReplace,
  Trade,
};
enum class Side { Buy, Sell };

// A single flattened representation of every ITCH 5.0 message this parser
// understands. Only the fields relevant to `type` are meaningful; everything
// else is left at its default-constructed value. This trades a larger struct
// for a uniform interface, the same way `Message` normalizes the CSV
// order-entry format in parser.hpp.
struct NasdaqMessage {
  NasdaqMessageType type;
  uint16_t stock_locate =
      0; // NASDAQ session-scoped locate code, not the ticker
  uint64_t timestamp =
      0; // nanoseconds since midnight, from a 48-bit wire field

  // OrderAdd / OrderAddWithMPID / OrderReplace / OrderExecute*
  // / OrderCancel / OrderDelete / Trade
  uint64_t order_id = 0; // for OrderReplace: the *new* order reference number
  uint64_t prev_order_id =
      0; // OrderReplace only: the order reference number being replaced
  Side side = Side::Buy; // OrderAdd / OrderAddWithMPID / Trade only
  uint32_t quantity = 0;
  FixedPoint<4> price{};
  std::string attribution; // OrderAddWithMPID only: 4-char MPID

  // StockDirectory
  std::string symbol; // ticker, up to 8 chars (also populated by MarketMaker)
  char market_category = '\0';
  char financial_status_indicator = '\0';
  uint32_t round_lot_size = 0;
  bool round_lots_only = false;
  char issue_classification = '\0';
  std::string issue_sub_type; // 2 chars
  char authenticity = '\0';
  bool short_sale_threshold_indicator = false;
  bool ipo_flag = false;
  char luld_reference_price_tier = '\0';
  bool etp_flag = false;
  uint32_t etp_leverage_factor = 0;
  bool inverse_indicator = false;

  // MarketMaker (Market Participant Position)
  std::string mpid; // 4-char market participant ID
  bool is_primary_market_maker = false;
  char market_maker_mode = '\0';
  char market_participant_state = '\0';
};

std::ostream &operator<<(std::ostream &os, const NasdaqMessage &msg);

// `pkg` is one fully-framed ITCH message payload with the 2-byte length
// prefix already stripped, i.e. pkg[0] is the message type byte — matching
// the framing `offset+2 .. offset+msgLen+2` used by the reference parser.
Expected<NasdaqMessage, std::string>
parse_message(std::span<const uint8_t> data, size_t &offset);

template <typename Book>
void processNasdaqMessage(Book &book, const NasdaqMessage &msg) {
  switch (msg.type) {
  case NasdaqMessageType::OrderAdd:
  case NasdaqMessageType::OrderAddWithMPID: {
    const auto side = msg.side == Side::Buy ? Book::Side::BID : Book::Side::ASK;
    auto added = book.newOrder(msg.order_id, side, msg.price, msg.quantity);
    if (!added)
      std::cout << "ERROR " << "Adding new order failed" << nl;
    break;
  }
  case NasdaqMessageType::OrderDelete: {
    auto cancelled = book.cancel(msg.order_id);
    if (!cancelled)
      std::cout << "ERROR " << "Cancelling existing order failed" << nl;
    break;
  }
  case NasdaqMessageType::OrderCancel:
  case NasdaqMessageType::OrderExecute:
  case NasdaqMessageType::OrderExecuteWithPrice: {
    auto amended = book.amendDelta(msg.order_id, msg.quantity);
    if (!amended)
      std::cout << "ERROR " << "Amending order failed" << nl;
    break;
  }
  case NasdaqMessageType::OrderReplace: {
    auto replaced = book.replaceOrder(msg.order_id, msg.price, msg.quantity);
    if (!replaced)
      std::cout << "ERROR " << "Replacing order failed" << nl;
    break;
  }
  case NasdaqMessageType::Trade:
  case NasdaqMessageType::StockDirectory:
  case NasdaqMessageType::MarketMaker:
  default:
    // Not order-book mutations: Trade executes against non-displayed
    // liquidity, the other two are reference data. Nothing to do here.
    break;
  }
}

} // namespace nasdaq

} // namespace parser
