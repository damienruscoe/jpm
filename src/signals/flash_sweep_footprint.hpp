#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #75 — Flash Sweep Volumetric Footprint
// L1 | P, Q | Core
// =============================================================================
//
// Detects and accumulates the total notional volume consumed during a rapid
// directional price sequence (a "sweep"). A sweep is defined as N or more
// consecutive trades in the same price direction. The signal reports the
// total quantity and notional value consumed during the most recently
// completed sweep sequence.
//
template <typename Traits> struct FlashSweepFootprint {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  int m_min_sweep_legs;

  std::optional<Price> m_last_price{};
  int m_streak{0};
  bool m_streak_direction_up{true};
  Price m_streak_qty{0};
  Price m_streak_notional{0};

  std::optional<Price> m_sweep_volume{};
  std::optional<Price> m_sweep_notional{};

  explicit FlashSweepFootprint(int min_sweep_legs = 3)
      : m_min_sweep_legs(min_sweep_legs) {}

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    const Price notional = event.price * qty;

    if (!m_last_price) {
      m_last_price = event.price;
      m_streak = 1;
      m_streak_qty = qty;
      m_streak_notional = notional;
      return;
    }

    const bool moved_up = event.price > *m_last_price;
    const bool moved_down = event.price < *m_last_price;
    const bool continued = (m_streak_direction_up && moved_up) ||
                           (!m_streak_direction_up && moved_down);

    if (continued) {
      m_streak++;
      m_streak_qty += qty;
      m_streak_notional += notional;
    } else {
      if (m_streak >= m_min_sweep_legs) {
        m_sweep_volume = m_streak_qty;
        m_sweep_notional = m_streak_notional;
      }
      m_streak_direction_up = moved_up;
      m_streak = 1;
      m_streak_qty = qty;
      m_streak_notional = notional;
    }

    m_last_price = event.price;
  }

  /**
   * @brief Returns the total volume consumed during the last detected sweep.
   *
   * A sweep is a run of consecutive trades in the same price direction
   * meeting or exceeding the configured minimum leg count. Large sweep
   * volumes relative to resting depth indicate aggressive liquidity
   * consumption and are a precursor to short-term momentum continuation.
   * Nullopt if no qualifying sweep has yet completed.
   */
  std::optional<Price> getLastSweepVolume() const { return m_sweep_volume; }

  /**
   * @brief Returns the total notional value consumed during the last sweep.
   */
  std::optional<Price> getLastSweepNotional() const { return m_sweep_notional; }
};
} // namespace signals
