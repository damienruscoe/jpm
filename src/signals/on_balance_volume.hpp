#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #121 — On-Balance Volume (OBV)
// L1 | P, Q | Core
// =============================================================================
//
// A directional volume accumulator: the full trade quantity is added to the
// running total on an up-tick and subtracted on a down-tick. No volume is
// added on a flat tick. The slope and divergence of the OBV line against
// price are the primary signal components.
//
template <typename Traits> struct OnBalanceVolume {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_last_price{};
  Price m_obv{0};
  bool m_has_first_trade{false};

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);

    if (!m_last_price) {
      m_last_price = event.price;
      m_has_first_trade = true;
      return;
    }

    if (event.price > *m_last_price)
      m_obv += qty;
    else if (event.price < *m_last_price)
      m_obv -= qty;

    m_last_price = event.price;
  }

  /**
   * @brief Returns the current On-Balance Volume accumulator.
   *
   * OBV is a leading indicator: smart money accumulation or distribution
   * typically shifts OBV before it manifests in price. A rising OBV with
   * flat price signals quiet accumulation; falling OBV into a rising price
   * signals distribution ahead of a potential reversal.
   * Nullopt until the first trade has established a price reference.
   */
  std::optional<Price> getObv() const {
    return m_has_first_trade ? std::optional<Price>{m_obv} : std::nullopt;
  }
};
} // namespace signals
