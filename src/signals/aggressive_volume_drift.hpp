#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #92 — Aggressive Volume Weighted Drift
// L1 | P, Q | Core
// =============================================================================
//
// Measures directional price drift, weighting each price change by the
// volume of the trade that produced it. Tick rule classifies whether each
// trade is buy- or sell-initiated, and the signed price delta is scaled by
// quantity before being accumulated into a rolling EMA. A positive value
// indicates upward drift driven by heavy buy-side aggression; negative
// indicates sell-side pressure.
//
template <typename Traits> struct AggressiveVolumeDrift {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  Price m_alpha;
  std::optional<Price> m_last_price{};
  std::optional<Price> m_ema_drift{};

  explicit AggressiveVolumeDrift(Price alpha = Price{5} / Price{100})
      : m_alpha(alpha) {}

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    const Price one = Price{1};

    if (!m_last_price) {
      m_last_price = event.price;
      return;
    }

    const Price delta = event.price - *m_last_price;
    const bool is_buy = delta >= Price{0};
    const Price sign = is_buy ? Price{1} : Price{-1};
    const Price raw_drift = sign * (delta < Price{0} ? -delta : delta) * qty;

    m_ema_drift =
        (one - m_alpha) * m_ema_drift.value_or(raw_drift) + m_alpha * raw_drift;
    m_last_price = event.price;
  }

  /**
   * @brief Returns the EMA of volume-weighted signed price drift.
   *
   * Positive values indicate that recent price moves accompanied by large
   * volume have been upward — a sign of buy-side aggression. Negative
   * values indicate sell-side pressure dominating the price formation
   * process. The magnitude reflects both the size and the speed of
   * directional commitment from market participants.
   * Nullopt until at least two trades have been processed.
   */
  std::optional<Price> getVolumeDrift() const { return m_ema_drift; }
};
} // namespace signals
