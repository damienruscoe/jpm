#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #118 — Accumulation/Distribution Line (A/D Line)
// L1 | P, Q | Core
// =============================================================================
//
// The classic A/D line requires a bar's high, low, and close to compute the
// Close Location Value (CLV). At tick level, successive trade prices act as
// the close, and the prior trade price is the high or low of the micro-bar.
// The CLV collapses to the sign of the tick direction, so raw money flow is
// signed and accumulated. The slope of this cumulative line is the primary
// actionable output.
//
template <typename Traits> struct AccumulationDistributionLine {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_last_price{};
  Price m_ad_line{0};
  std::optional<Price> m_prev_ad_line{};
  bool m_has_first_trade{false};

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    const Price money_flow = event.price * qty;

    if (!m_last_price) {
      m_last_price = event.price;
      m_has_first_trade = true;
      return;
    }

    m_prev_ad_line = m_ad_line;

    // Tick-level CLV: +1 if price rose, -1 if price fell, 0 if unchanged
    if (event.price > *m_last_price)
      m_ad_line += money_flow;
    else if (event.price < *m_last_price)
      m_ad_line -= money_flow;

    m_last_price = event.price;
  }

  /**
   * @brief Returns the current cumulative Accumulation/Distribution value.
   *
   * A rising A/D line indicates that volume is accumulating on up-ticks —
   * net buying pressure. A falling line indicates distribution — volume
   * flowing on down-ticks. Divergence between the A/D line and price is a
   * classic signal of hidden accumulation or distribution by informed
   * participants ahead of a directional move.
   */
  std::optional<Price> getAdLine() const {
    return m_has_first_trade ? std::optional<Price>{m_ad_line} : std::nullopt;
  }

  /**
   * @brief Returns the one-trade slope of the A/D line (current minus prior).
   */
  std::optional<Price> getAdSlope() const {
    if (m_prev_ad_line)
      return m_ad_line - *m_prev_ad_line;
    return std::nullopt;
  }
};
} // namespace signals
