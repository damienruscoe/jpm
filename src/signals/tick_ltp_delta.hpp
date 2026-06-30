#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #62 — Tick-Based Last Trade Price (LTP) Delta
// L1 | P | Core
// =============================================================================
//
// Description
//
template <typename Traits> struct TickLtpDelta {
  using Price = typename Traits::Price;

  std::optional<Price> m_prev_price{};
  std::optional<Price> m_delta{};

  void update(const TradeEvent<Traits> &event) {
    if (m_prev_price) {
      m_delta = event.price - *m_prev_price;
    }
    m_prev_price = event.price;
  }

  std::optional<Price> getLtpDelta() const { return m_delta; }
};
} // namespace signals
