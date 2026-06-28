#pragma once
#include "trade_event.hpp"

namespace signals {
template <typename Traits> struct EmptySignals {
  void update(const TradeEvent<Traits> &) {}
};
} // namespace signals
