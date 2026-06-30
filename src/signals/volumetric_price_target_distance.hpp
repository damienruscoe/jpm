#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #135 — Volumetric Price Target Distance
// L1 | P, Q | Core
// =============================================================================
//
// Computes the volume-weighted centre of the trade distribution (the volume
// node) over the session and returns the signed distance between the current
// trade price and that centre. This is the tick-level analogue of the
// Point-of-Control (POC) distance in Market Profile analysis. A positive
// value means the current price is above the session's volume-weighted
// centre of gravity; negative means it is below.
//
template <typename Traits> struct VolumetricPriceTargetDistance {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  Price m_cum_notional{0};
  Price m_cum_volume{0};
  std::optional<Price> m_last_price{};

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    m_cum_notional += event.price * qty;
    m_cum_volume += qty;
    m_last_price = event.price;
  }

  /**
   * @brief Returns the session VWAP (volume-node centre of gravity).
   *
   * The volume node is defined as the cumulative VWAP — the price level
   * at which the majority of volume has transacted. This anchors the
   * target distance calculation.
   */
  std::optional<Price> getVolumeNode() const {
    if (m_cum_volume > Price{0})
      return m_cum_notional / m_cum_volume;
    return std::nullopt;
  }

  /**
   * @brief Returns the signed distance from the current price to the volume
   * node.
   *
   * Positive values indicate the current price is trading at a premium to
   * the session's volume-weighted fair value. Negative values indicate a
   * discount. Mean-reversion strategies use this distance as a primary
   * entry trigger; momentum strategies use persistent large positive or
   * negative readings as a trend confirmation signal.
   * Nullopt until the first trade has been processed.
   */
  std::optional<Price> getPriceTargetDistance() const {
    const auto node = getVolumeNode();
    if (node && m_last_price)
      return *m_last_price - *node;
    return std::nullopt;
  }
};
} // namespace signals
