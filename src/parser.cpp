#include "parser.hpp"

#include <array>
#include <charconv>
#include <iomanip>

// clang-format off
static constexpr std::array<std::string_view, 4> REQUEST_TYPE_STRINGS = { "N", "C", "A"};
static constexpr std::array<std::string_view, 3> SIDE_STRINGS = {"B", "S"};

std::ostream &operator<<(std::ostream &os, const Message &msg) {
  os << "Ticker: " << msg.exchange_ticker
     << " | Type: " << REQUEST_TYPE_STRINGS[static_cast<size_t>(msg.type)]
     << " | ID: " << std::setw(10) << msg.order_id
     << " | Side: " << SIDE_STRINGS[static_cast<size_t>(msg.side)]
     << " | Qty: " << msg.quantity
     << " | Price: " << std::fixed
     << std::setprecision(2) << msg.price.ToDouble();
  return os;
}
// clang-format on

static constexpr std::string_view WHITESPACE = " \t\r\n\v\f";

std::pair<std::string_view, std::string_view> split(std::string_view s,
                                                    char delim) {
  size_t pos = s.find(delim);
  if (pos == std::string_view::npos)
    return std::pair{s, std::string_view{}};
  return std::pair{s.substr(0, pos), s.substr(pos + 1)};
};

std::string_view trim_inline_comments(std::string_view str) {
  size_t comment_pos = str.find("//");
  if (comment_pos != std::string_view::npos)
    str = str.substr(0, comment_pos);
  return str;
}

std::string_view trim_whitespace_prefix(std::string_view str) {
  size_t first = str.find_first_not_of(WHITESPACE);
  if (first == std::string_view::npos)
    return {};
  str.remove_prefix(first);
  return str;
}

std::string_view trim_whitespace_suffix(std::string_view str) {
  size_t last = str.find_last_not_of(WHITESPACE);
  str.remove_suffix(str.size() - last - 1);
  return str;
}

std::string_view trim_whitespace(std::string_view str) {
  return trim_whitespace_suffix(trim_whitespace_prefix(str));
}

std::optional<Message> parse_line(std::string_view line) {
  auto parse_integer = [](std::string_view str, uint32_t &output) {
    auto [ptr, ec] =
        std::from_chars(str.data(), str.data() + str.size(), output);
    return ec == std::errc{} && ptr == str.data() + str.size();
  };

  auto error = [](std::string_view message, std::string_view details) {
    std::cerr << "[\033[31mERROR\033[0m] " << message << ": \033[36m" << '"'
              << details << "\"\033[0m" << std::endl;
    return std::nullopt;
  };

  line = trim_inline_comments(line);
  line = trim_whitespace_suffix(line);

  auto [ticker_str, rest1] = split(line, ',');
  auto [type_str, rest2] = split(rest1, ',');
  auto [id_str, rest3] = split(rest2, ',');
  auto [side_str, rest4] = split(rest3, ',');
  auto [qty_str, price_str] = split(rest4, ',');

  Message msg;

  // 1. Ticker
  if (!parse_integer(ticker_str, msg.exchange_ticker))
    return error("Invalid ticker", ticker_str);
  if (msg.exchange_ticker <= 0)
    return error("Ticker must be positive", ticker_str);

  // 2. Type
  if (type_str == "N")
    msg.type = RequestType::New;
  else if (type_str == "C")
    msg.type = RequestType::Cancel;
  else if (type_str == "A")
    msg.type = RequestType::Amend;
  else
    return error("Invalid request type", type_str);

  // 3. Order ID
  if (id_str.empty())
    return error("Missing ID", id_str);
  if (id_str.size() > 10)
    return error("Order ID too long", id_str);
  for (char c : id_str) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-')
      return error("Invalid character in order ID", id_str);
  }
  msg.order_id = id_str;

  // 4. Side
  if (side_str == "B")
    msg.side = Side::Buy;
  else if (side_str == "S")
    msg.side = Side::Sell;
  else
    return error("Invalid side", side_str);

  // 5. Quantity
  if (!parse_integer(qty_str, msg.quantity))
    return error("Invalid quantity", qty_str);
  if (msg.quantity <= 0)
    return error("Quantity must be positive", qty_str);

  // 6. Price
  // if (price_str.find(',') != std::string_view::npos)
  //  return error("Too many columns", line);
  try {
    msg.price = FixedPoint::Parse(price_str);
  } catch (...) {
    return error("Invalid price", price_str);
  }

  return msg;
}
