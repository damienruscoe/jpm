#pragma once

#include "enums.hpp"

template <typename Traits> struct OrderMatchedEvent {
  typename Traits::OrderID order_id;
  typename Traits::Price price;
  typename Traits::Quantity quantity;
  typename Traits::Quantity remaining;
  side_t side;
};

template <typename Traits> struct TradeEvent {
  typename Traits::OrderID order_id;
  typename Traits::Price price;
  typename Traits::Quantity quantity;
  side_t aggressor_side;
  FillStatus fill;
};

template <typename Traits> struct LevelQuantityEvent {
  typename Traits::Price price;
  typename Traits::Quantity quantity;
  side_t side;
};
