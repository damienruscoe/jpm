#pragma once

#include "enums.hpp"

template <typename Traits> struct TradeEvent {
  typename Traits::OrderID order_id;
  typename Traits::Price price;
  typename Traits::Quantity quantity;
  side_t aggressor_side;
  FillStatus fill;
};
