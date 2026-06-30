#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #122 — Ease of Movement (EOM)
// L1 | P, Q | Core
// =============================================================================
//
// EOM relates the rate of price change to the volume required to produce it.
// In a bar-based context, the midpoint move is divided by a box ratio derived
// from volume and high-low range. At tick level, the price delta replaces the
// midpoint move, and the trade quantity replaces the box ratio denominator.
// A smoothed EMA of raw EOM is provided as the primary signal.
//
template <typename Traits> struct EaseOfMovement {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  Price m_alpha;
  std::optional<Price> m_last_price{};
  std::optional<Price> m_ema_eom{};

  explicit EaseOfMovement(Price alpha = Price{5} / Price{100})
      : m_alpha(alpha) {}

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    const Price one = Price{1};

    if (!m_last_price) {
      m_last_price = event.price;
      return;
    }

    if (qty > Price{0}) {
      const Price delta = event.price - *m_last_price;
      const Price raw_eom = delta / qty;
      m_ema_eom =
          (one - m_alpha) * m_ema_eom.value_or(raw_eom) + m_alpha * raw_eom;
    }

    m_last_price = event.price;
  }

  /**
   * @brief Returns the EMA-smoothed Ease of Movement value.
   *
   * Large positive EOM indicates price is rising easily on light volume —
   * an efficient, low-resistance upward move typical of strong momentum.
   * Large negative EOM indicates effortless downward movement. Values near
   * zero indicate price is either flat or requires heavy volume to move,
   * implying resistance or congestion. Nullopt until two trades are seen.
   */
  std::optional<Price> getEom() const { return m_ema_eom; }
};
} // namespace signals
