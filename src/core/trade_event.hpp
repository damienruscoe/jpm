#pragma once

template <typename Traits> struct TradeEvent {
  typename Traits::Price price;
  typename Traits::Quantity quantity;
};
