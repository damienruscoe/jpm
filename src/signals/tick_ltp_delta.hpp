#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
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
