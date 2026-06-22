#pragma once

#include "FixedPointGeneric.hpp"

#include <iostream>
#include <optional>
#include <string_view>

enum class RequestType { New, Cancel, Amend };
enum class Side { Buy, Sell };

struct Message {
  uint32_t exchange_ticker;
  RequestType type;
  std::string_view order_id;
  Side side;
  uint32_t quantity;
  FixedPointGeneric price;
};

std::ostream &operator<<(std::ostream &os, const Message &msg);

std::pair<std::string_view, std::string_view> split(std::string_view s,
                                                    char delim);
std::string_view trim_inline_comments(std::string_view str);
std::string_view trim_whitespace(std::string_view str);
std::string_view trim_whitespace_prefix(std::string_view str);
std::string_view trim_whitespace_suffix(std::string_view str);

std::optional<Message> parse_line(std::string_view line);
