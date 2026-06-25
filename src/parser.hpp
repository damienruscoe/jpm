#pragma once

#include "expected.hpp"
#include "fixed_point.hpp"

#include <iostream>
#include <string_view>

enum class RequestType { New, Cancel, Amend };
enum class Side { Buy, Sell };

struct Message {
  uint32_t exchange_ticker;
  RequestType type;
  std::string_view order_id;
  Side side;
  uint32_t quantity;
  FixedPoint<4> price;
};

std::ostream &operator<<(std::ostream &os, const Message &msg);

Expected<Message, std::string_view> parse_line(std::string_view line);
