#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #49 — Cumulative Volume Delta (CVD)
// L2 (table) | Q | Core  — implemented here from trade prints only
// =============================================================================
//
// The table classifies CVD as an L2 signal, but in its canonical form it is a
// running sum of signed trade volume: aggressive buy volume minus aggressive
// sell volume, accumulated since the start of the session. This requires only
// trade prints and a tick-rule aggressor classification — no book depth is
// needed — so it is implemented here against TradeEvent rather than against
// OrderBook.
//
template <typename Traits> struct CumulativeVolumeDelta {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_last_price{};
  Price m_cvd{0};
  bool m_has_first_trade{false};

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);

    if (!m_last_price) {
      m_last_price = event.price;
      m_has_first_trade = true;
      // First trade has no prior price to classify direction from; treat
      // as buy-initiated by convention, matching the tick-rule default used
      // elsewhere in this file (e.g. VpinSignal, BuySellLotRatio).
      m_cvd += qty;
      return;
    }

    const bool is_buy = event.price >= *m_last_price;
    m_cvd += is_buy ? qty : -qty;
    m_last_price = event.price;
  }

  /**
   * @brief Returns the session-cumulative volume delta.
   *
   * A running total of aggressive buy volume minus aggressive sell volume,
   * classified via the tick rule (trade at or above prior price = buy
   * aggressor). A persistently rising CVD confirms buy-side dominance even
   * when price is range-bound — often a precursor to an upside breakout.
   * A falling CVD into flat or rising price is a classic distribution
   * divergence pattern. Nullopt until the first trade has been processed.
   */
  std::optional<Price> getCvd() const {
    return m_has_first_trade ? std::optional<Price>{m_cvd} : std::nullopt;
  }
};
} // namespace signals
