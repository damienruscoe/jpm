#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #70 — Tick Run Length (Directional Persistence)
// L1 | P | Core
// =============================================================================
//
// Description
//
template <typename Traits> struct TickRunLength {
  using Price = typename Traits::Price;

  std::optional<Price> m_prev_price{};
  int m_run_length{0};

  void update(const TradeEvent<Traits> &event) {
    if (m_prev_price) {
      if (event.price > *m_prev_price) {
        m_run_length = (m_run_length > 0) ? m_run_length + 1 : 1;
      } else if (event.price < *m_prev_price) {
        m_run_length = (m_run_length < 0) ? m_run_length - 1 : -1;
      } else {
        m_run_length = 0;
      }
    }
    m_prev_price = event.price;
  }

  int getRunLength() const { return m_run_length; }
};
} // namespace signals
