#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #120 — Volume Price Trend (VPT)
// L1 | P, Q | Core
// =============================================================================
//
// Adds or subtracts a fraction of the current volume to a running total,
// where the fraction is the percentage price change from the prior trade.
// Unlike OBV, VPT is sensitive to the magnitude of the price move, making
// it more responsive to large-gap executions.
//
template <typename Traits> struct VolumePriceTrend {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_last_price{};
  Price m_vpt{0};
  bool m_has_first_trade{false};

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);

    if (!m_last_price) {
      m_last_price = event.price;
      m_has_first_trade = true;
      return;
    }

    if (*m_last_price > Price{0}) {
      const Price pct_change = (event.price - *m_last_price) / *m_last_price;
      m_vpt += qty * pct_change;
    }

    m_last_price = event.price;
  }

  /**
   * @brief Returns the current cumulative Volume Price Trend value.
   *
   * A rising VPT indicates that volume is flowing in on positive price
   * moves — confirming bullish momentum. A falling VPT indicates selling
   * pressure is volume-confirmed. VPT divergence from price (price making
   * new highs while VPT fails to) is a leading indicator of exhaustion.
   * Nullopt until the first trade has established a price reference.
   */
  std::optional<Price> getVpt() const {
    return m_has_first_trade ? std::optional<Price>{m_vpt} : std::nullopt;
  }
};
} // namespace signals
