#include "parser/nasdaq.hpp"
#include "str_utils.hpp"
#include <array>
#include <iomanip>
#include <sstream>

using namespace parser::nasdaq;

namespace {

constexpr size_t STOCK_DIRECTORY_LEN = 39;
constexpr size_t MARKET_MAKER_LEN = 26;
constexpr size_t ORDER_ADD_LEN = 36;
constexpr size_t ORDER_ADD_MPID_LEN = 40;
constexpr size_t ORDER_EXECUTE_LEN = 23;
constexpr size_t ORDER_EXECUTE_PRICE_LEN = 36;
constexpr size_t ORDER_CANCEL_LEN = 23;
constexpr size_t ORDER_DELETE_LEN = 19;
constexpr size_t ORDER_REPLACE_LEN = 35;
constexpr size_t TRADE_LEN = 36;

// clang-format off
constexpr std::array<std::string_view, 10> NASDAQ_MESSAGE_TYPE_STRINGS = {
    "StockDirectory", "MarketMaker",           "OrderAdd", "OrderAddWithMPID",
    "OrderExecute",   "OrderExecuteWithPrice", "OrderCancel", "OrderDelete",
    "OrderReplace",   "Trade"};
// clang-format on

uint16_t read_u16(std::span<const uint8_t> pkg, size_t offset) {
  return static_cast<uint16_t>((pkg[offset] << 8) | pkg[offset + 1]);
}

uint32_t read_u32(std::span<const uint8_t> pkg, size_t offset) {
  return (static_cast<uint32_t>(pkg[offset]) << 24) |
         (static_cast<uint32_t>(pkg[offset + 1]) << 16) |
         (static_cast<uint32_t>(pkg[offset + 2]) << 8) |
         static_cast<uint32_t>(pkg[offset + 3]);
}

uint64_t read_u48(std::span<const uint8_t> pkg, size_t offset) {
  uint64_t value = 0;
  for (size_t i = 0; i < 6; ++i)
    value = (value << 8) | pkg[offset + i];
  return value;
}

uint64_t read_u64(std::span<const uint8_t> pkg, size_t offset) {
  uint64_t value = 0;
  for (size_t i = 0; i < 8; ++i)
    value = (value << 8) | pkg[offset + i];
  return value;
}

// Fixed-width ASCII fields are right-padded with spaces on the wire.
std::string read_ascii(std::span<const uint8_t> pkg, size_t offset,
                       size_t len) {
  std::string s(reinterpret_cast<const char *>(pkg.data() + offset), len);
  while (!s.empty() && s.back() == ' ')
    s.pop_back();
  return s;
}

bool read_flag(uint8_t c) { return c == 'Y'; }

Side read_side(uint8_t c) { return c == 'B' ? Side::Buy : Side::Sell; }

// ITCH prices are transmitted as integers with 4 implied decimal digits,
// which is exactly what FixedPoint<4> represents. We go via Parse() rather
// than assuming anything about FixedPoint's internal storage; if FixedPoint
// exposes a raw-scaled-integer constructor, swap this for that on the hot
// path to avoid the string round-trip.
FixedPoint<4> price_from_raw(uint32_t raw) {
  std::ostringstream ss;
  ss << (raw / 10000) << '.' << std::setw(4) << std::setfill('0')
     << (raw % 10000);
  auto parsed = FixedPoint<4>::Parse(ss.str());
  return *parsed; // constructed from a format we control; cannot fail
}

auto error(std::string_view context, std::string_view details) {
  std::ostringstream ss;
  ss << context << ": " << details;
  return Expected<NasdaqMessage, std::string>(ss.str());
}

Expected<NasdaqMessage, std::string>
handle_stock_directory(std::span<const uint8_t> pkg) {
  if (pkg.size() < STOCK_DIRECTORY_LEN)
    return error("Stock Directory", "message too short");
  NasdaqMessage msg;
  msg.type = NasdaqMessageType::StockDirectory;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.symbol = read_ascii(pkg, 11, 8);
  msg.market_category = static_cast<char>(pkg[19]);
  msg.financial_status_indicator = static_cast<char>(pkg[20]);
  msg.round_lot_size = read_u32(pkg, 21);
  msg.round_lots_only = read_flag(pkg[25]);
  msg.issue_classification = static_cast<char>(pkg[26]);
  msg.issue_sub_type = read_ascii(pkg, 27, 2);
  msg.authenticity = static_cast<char>(pkg[29]);
  msg.short_sale_threshold_indicator = read_flag(pkg[30]);
  msg.ipo_flag = read_flag(pkg[31]);
  msg.luld_reference_price_tier = static_cast<char>(pkg[32]);
  msg.etp_flag = read_flag(pkg[33]);
  msg.etp_leverage_factor = read_u32(pkg, 34);
  msg.inverse_indicator = read_flag(pkg[38]);
  return msg;
}

Expected<NasdaqMessage, std::string>
handle_market_maker(std::span<const uint8_t> pkg) {
  if (pkg.size() < MARKET_MAKER_LEN)
    return error("Market Maker", "message too short");
  NasdaqMessage msg;
  msg.type = NasdaqMessageType::MarketMaker;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.mpid = read_ascii(pkg, 11, 4);
  msg.symbol = read_ascii(pkg, 15, 8);
  msg.is_primary_market_maker = read_flag(pkg[23]);
  msg.market_maker_mode = static_cast<char>(pkg[24]);
  msg.market_participant_state = static_cast<char>(pkg[25]);
  return msg;
}

Expected<NasdaqMessage, std::string>
handle_order_add(std::span<const uint8_t> pkg, bool with_mpid) {
  const size_t min_len = with_mpid ? ORDER_ADD_MPID_LEN : ORDER_ADD_LEN;
  const std::string_view context = with_mpid ? "Order Add (MPID)" : "Order Add";
  if (pkg.size() < min_len)
    return error(context, "message too short");
  if (pkg[19] != 'B' && pkg[19] != 'S')
    return error(context, "invalid side");
  NasdaqMessage msg;
  msg.type = with_mpid ? NasdaqMessageType::OrderAddWithMPID
                       : NasdaqMessageType::OrderAdd;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.order_id = read_u64(pkg, 11);
  msg.side = read_side(pkg[19]);
  msg.quantity = read_u32(pkg, 20);
  msg.price = price_from_raw(read_u32(pkg, 32));
  if (with_mpid)
    msg.attribution = read_ascii(pkg, 36, 4);
  return msg;
}

Expected<NasdaqMessage, std::string>
handle_order_execute(std::span<const uint8_t> pkg) {
  if (pkg.size() < ORDER_EXECUTE_LEN)
    return error("Order Execute", "message too short");
  NasdaqMessage msg;
  msg.type = NasdaqMessageType::OrderExecute;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.order_id = read_u64(pkg, 11);
  msg.quantity = read_u32(pkg, 19);
  return msg;
}

Expected<NasdaqMessage, std::string>
handle_order_execute_with_price(std::span<const uint8_t> pkg) {
  if (pkg.size() < ORDER_EXECUTE_PRICE_LEN)
    return error("Order Execute (Price)", "message too short");
  NasdaqMessage msg;
  msg.type = NasdaqMessageType::OrderExecuteWithPrice;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.order_id = read_u64(pkg, 11);
  msg.quantity = read_u32(pkg, 19);
  msg.price = price_from_raw(read_u32(pkg, 32));
  return msg;
}

Expected<NasdaqMessage, std::string>
handle_order_cancel(std::span<const uint8_t> pkg) {
  if (pkg.size() < ORDER_CANCEL_LEN)
    return error("Order Cancel", "message too short");
  NasdaqMessage msg;
  msg.type = NasdaqMessageType::OrderCancel;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.order_id = read_u64(pkg, 11);
  msg.quantity = read_u32(pkg, 19);
  return msg;
}

Expected<NasdaqMessage, std::string>
handle_order_delete(std::span<const uint8_t> pkg) {
  if (pkg.size() < ORDER_DELETE_LEN)
    return error("Order Delete", "message too short");
  NasdaqMessage msg;
  msg.type = NasdaqMessageType::OrderDelete;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.order_id = read_u64(pkg, 11);
  return msg;
}

Expected<NasdaqMessage, std::string>
handle_order_replace(std::span<const uint8_t> pkg) {
  if (pkg.size() < ORDER_REPLACE_LEN)
    return error("Order Replace", "message too short");
  NasdaqMessage msg;
  msg.type = NasdaqMessageType::OrderReplace;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.prev_order_id = read_u64(pkg, 11);
  msg.order_id = read_u64(pkg, 19);
  msg.quantity = read_u32(pkg, 27);
  msg.price = price_from_raw(read_u32(pkg, 31));
  return msg;
}

Expected<NasdaqMessage, std::string>
handle_trade(std::span<const uint8_t> pkg) {
  if (pkg.size() < TRADE_LEN)
    return error("Trade", "message too short");
  if (pkg[19] != 'B' && pkg[19] != 'S')
    return error("Trade", "invalid side");
  NasdaqMessage msg;
  msg.type = NasdaqMessageType::Trade;
  msg.stock_locate = read_u16(pkg, 1);
  msg.timestamp = read_u48(pkg, 5);
  msg.order_id = read_u64(pkg, 11);
  msg.side = read_side(pkg[19]);
  msg.quantity = read_u32(pkg, 20);
  msg.price = price_from_raw(read_u32(pkg, 32));
  return msg;
}

} // namespace

std::ostream &parser::nasdaq::operator<<(std::ostream &os,
                                         const NasdaqMessage &msg) {
  os << fmt::KeyValue{"Type", NASDAQ_MESSAGE_TYPE_STRINGS[static_cast<size_t>(
                                  msg.type)]}
     << fmt::KeyValue{"Locate", msg.stock_locate}
     << fmt::KeyValue{"TS", msg.timestamp};
  switch (msg.type) {
  case NasdaqMessageType::StockDirectory:
    os << fmt::KeyValue{"Symbol", msg.symbol}
       << fmt::KeyValue{"RoundLot", msg.round_lot_size};
    break;
  case NasdaqMessageType::MarketMaker:
    os << fmt::KeyValue{"MPID", msg.mpid} << fmt::KeyValue{"Symbol", msg.symbol}
       << fmt::KeyValue{"Primary", msg.is_primary_market_maker};
    break;
  case NasdaqMessageType::OrderAdd:
  case NasdaqMessageType::OrderAddWithMPID:
    os << fmt::KeyValue{"ID", msg.order_id}
       << fmt::KeyValue{"Side", (msg.side == Side::Buy ? 'B' : 'S')}
       << fmt::KeyValue{"Qty", msg.quantity}
       << fmt::KeyValue{"Price", msg.price};
    if (msg.type == NasdaqMessageType::OrderAddWithMPID)
      os << fmt::KeyValue{"Attribution", msg.attribution};
    break;
  case NasdaqMessageType::OrderExecute:
    os << fmt::KeyValue{"ID", msg.order_id}
       << fmt::KeyValue{"Qty", msg.quantity};
    break;
  case NasdaqMessageType::OrderExecuteWithPrice:
    os << fmt::KeyValue{"ID", msg.order_id}
       << fmt::KeyValue{"Qty", msg.quantity}
       << fmt::KeyValue{"Price", msg.price};
    break;
  case NasdaqMessageType::OrderCancel:
    os << fmt::KeyValue{"ID", msg.order_id}
       << fmt::KeyValue{"Qty", msg.quantity};
    break;
  case NasdaqMessageType::OrderDelete:
    os << fmt::KeyValue{"ID", msg.order_id};
    break;
  case NasdaqMessageType::OrderReplace:
    os << fmt::KeyValue{"ID", msg.order_id}
       << fmt::KeyValue{"PrevID", msg.prev_order_id}
       << fmt::KeyValue{"Qty", msg.quantity}
       << fmt::KeyValue{"Price", msg.price};
    break;
  case NasdaqMessageType::Trade:
    os << fmt::KeyValue{"ID", msg.order_id}
       << fmt::KeyValue{"Side", (msg.side == Side::Buy ? 'B' : 'S')}
       << fmt::KeyValue{"Qty", msg.quantity}
       << fmt::KeyValue{"Price", msg.price};
    break;
  }
  return os;
}

Expected<NasdaqMessage, std::string>
parser::nasdaq::parse_message(std::span<const uint8_t> data, size_t &offset) {
  if (offset + 2 > data.size()) {
    offset = data.size(); // nothing left worth reading
    return error("Message", "truncated length prefix");
  }
  const uint16_t msg_len = read_u16(data, offset);
  const size_t frame_end = offset + 2 + msg_len;
  if (frame_end > data.size()) {
    offset =
        data.size(); // declared length runs past the buffer; can't resync, stop
    return error("Message", "truncated message body");
  }
  const std::span<const uint8_t> pkg = data.subspan(offset + 2, msg_len);
  offset = frame_end; // always advance past this frame, success or failure

  if (pkg.empty())
    return error("Message", "empty buffer");
  switch (static_cast<char>(pkg[0])) {
  case 'R':
    return handle_stock_directory(pkg);
  case 'L':
    return handle_market_maker(pkg);
  case 'A':
    return handle_order_add(pkg, false);
  case 'F':
    return handle_order_add(pkg, true);
  case 'E':
    return handle_order_execute(pkg);
  case 'C':
    return handle_order_execute_with_price(pkg);
  case 'X':
    return handle_order_cancel(pkg);
  case 'D':
    return handle_order_delete(pkg);
  case 'U':
    return handle_order_replace(pkg);
  case 'P':
    return handle_trade(pkg);
  default: {
    std::ostringstream ss;
    ss << "Unknown message type '" << static_cast<char>(pkg[0]) << "'";
    return Expected<NasdaqMessage, std::string>(ss.str());
  }
  }
}
