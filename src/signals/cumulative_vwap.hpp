#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #65.1 or 66.1 — Cumulative Volume WAP
// L1 | P, Q | Core
// =============================================================================
//
// Description
//
template <typename Traits> struct CumulativeVWAP {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_cum_value{};
  std::optional<Price> m_cum_volume{};

  void update(const TradeEvent<Traits> &event) {
    m_cum_value =
        m_cum_value.value_or(Price{0}) + (event.price * event.quantity);
    m_cum_volume =
        m_cum_volume.value_or(Price{0}) + static_cast<Price>(event.quantity);
  }

  /**
   * @brief Returns the session-wide cumulative volume.
   */
  std::optional<Price> getCumulativeVolume() const { return m_cum_volume; }

  /**
   * @brief Returns the session-wide cumulative value.
   */
  std::optional<Price> getCumulativeValue() const { return m_cum_value; }

  /**
   * @brief Returns the session-wide cumulative VWAP.
   *
   * Provides the absolute average price of all volume traded since the
   * start of the session. It serves as an objective anchor point for
   * evaluating execution performance.
   */
  std::optional<Price> getCumulativeVwap() const {
    if (m_cum_volume && *m_cum_volume > 0)
      return m_cum_value.value_or(Price{0}) / *m_cum_volume;
    return std::nullopt;
  }
};
} // namespace signals
