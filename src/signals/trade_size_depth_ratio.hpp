#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #86 — Trade Size vs Depth Cushion Ratio
// L1 | Q | Core
// =============================================================================
//
// Compares the incoming trade quantity against a configurable reference
// representing the inside-tier depth cushion. In an L1-only context the
// cushion is approximated by a rolling EMA of observed trade quantities,
// acting as a proxy for typical resting size at the inside touch. The ratio
// measures how deeply the current trade consumes that cushion estimate.
//
template <typename Traits> struct TradeSizeDepthRatio {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  Price m_alpha;
  std::optional<Price> m_ema_depth_proxy{};
  std::optional<Price> m_ratio{};

  explicit TradeSizeDepthRatio(Price alpha = Price{5} / Price{100})
      : m_alpha(alpha) {}

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    const Price one = Price{1};

    // Update depth proxy before computing ratio so ratio reflects the
    // cushion state prior to this trade consuming it.
    const Price prior_depth = m_ema_depth_proxy.value_or(qty);

    m_ema_depth_proxy = (one - m_alpha) * prior_depth + m_alpha * qty;

    if (prior_depth > Price{0})
      m_ratio = qty / prior_depth;
  }

  /**
   * @brief Returns the ratio of the current trade size to the depth proxy.
   *
   * Values significantly above 1 indicate that the incoming trade is
   * disproportionately large relative to the typical resting liquidity
   * at the inside touch, suggesting potential tier exhaustion or a sweep
   * initiation. Values near or below 1 indicate normal absorption.
   * Nullopt until the first trade has established the depth proxy.
   */
  std::optional<Price> getDepthConsumptionRatio() const { return m_ratio; }

  /**
   * @brief Returns the EMA-based inside depth proxy.
   */
  std::optional<Price> getDepthProxy() const { return m_ema_depth_proxy; }
};
} // namespace signals
