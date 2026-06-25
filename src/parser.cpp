#include "parser.hpp"

#include "str_utils.hpp"
#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

// clang-format off
static constexpr std::array<std::string_view, 3> REQUEST_TYPE_STRINGS = {"N", "C", "A"};
static constexpr std::array<char, 2> SIDE_STRINGS = {'B', 'S'};

std::ostream &operator<<(std::ostream &os, const Message &msg) {
  os << "Ticker: " << msg.exchange_ticker
     << " | Type: " << REQUEST_TYPE_STRINGS[static_cast<size_t>(msg.type)]
     << " | ID: " << std::setw(10) << msg.order_id
     << " | Side: " << SIDE_STRINGS[static_cast<size_t>(msg.side)]
     << " | Qty: " << msg.quantity
     << " | Price: " << msg.price;
  return os;
}
// clang-format on

auto error(std::string_view line, std::string_view error_msg,
           std::string_view details) {
  std::ostringstream ss;
  ss << error_msg << '(' << details << ')' << " " << line;
  return Expected<Message, std::string>(ss.str());
};

Expected<Message, std::string> parse_line(std::string_view line) {
  line = trim_inline_comments(line);
  line = trim_whitespace_suffix(line);

  auto [ticker_str, rest1] = split(line, ',');
  auto [type_str, rest2] = split(rest1, ',');
  auto [id_str, rest3] = split(rest2, ',');
  auto [side_str, rest4] = split(rest3, ',');
  auto [qty_str, price_str] = split(rest4, ',');

  Message msg;

  // Ticker
  if (!parse_integer(ticker_str, msg.exchange_ticker))
    return error(line, "Invalid ticker", ticker_str);

  // Type
  if (type_str == "N")
    msg.type = RequestType::New;
  else if (type_str == "C")
    msg.type = RequestType::Cancel;
  else if (type_str == "A")
    msg.type = RequestType::Amend;
  else
    return error(line, "Invalid request type", type_str);

  // Order ID
  msg.order_id = id_str;

  // Side
  if (side_str == "B")
    msg.side = Side::Buy;
  else if (side_str == "S")
    msg.side = Side::Sell;
  else
    return error(line, "Invalid side", side_str);

  // Quantity
  if (!parse_integer(qty_str, msg.quantity))
    return error(line, "Invalid quantity", qty_str);

  // Price
  if (auto parsed = FixedPoint<4>::Parse(price_str))
    msg.price = *parsed;
  else
    return error(line, "Invalid price", price_str);

  const auto valid_id_char = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '-';
  };

  if (msg.exchange_ticker <= 0)
    return error(line, "Ticker must be positive", ticker_str);

  if (msg.order_id.empty())
    return error(line, "Missing ID", id_str);
  if (msg.order_id.size() > 10)
    return error(line, "Order ID too long", id_str);
  if (!std::ranges::all_of(msg.order_id, valid_id_char))
    return error(line, "Invalid character in order ID", id_str);

  if (msg.quantity <= 0)
    return error(line, "Quantity must be positive", qty_str);

  if (msg.price <= FixedPoint<4>{0})
    return error(line, "Price must be positive", price_str);

  return msg;
}
