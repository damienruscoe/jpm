#pragma once

#include "trade_event.hpp"
#include <cmath>
#include <optional>

namespace signals {
// =============================================================================
// #69 — Aggressive Block Trade Detector
// L1 | Q | Core
// =============================================================================
//
// Flags individual trade executions whose quantity exceeds a rolling mean by
// a configurable multiple of the rolling standard deviation. The rolling
// statistics are maintained with Welford's online algorithm to avoid
// reprocessing the entire window on every tick.
//
template <typename Traits> struct BlockTradeDetector {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  int m_window;
  Price m_threshold_sigma;

  // Welford's online algorithm state
  int m_count{0};
  Price m_mean{0};
  Price m_m2{0};

  std::optional<bool> m_is_block_trade{};
  std::optional<Quantity> m_last_quantity{};

  explicit BlockTradeDetector(int window = 100,
                              Price threshold_sigma = Price{3})
      : m_window(window), m_threshold_sigma(threshold_sigma) {}

  void update(const TradeEvent<Traits> &event) {
    m_last_quantity = event.quantity;
    const Price qty = static_cast<Price>(event.quantity);

    m_count++;
    const Price delta = qty - m_mean;
    m_mean += delta / static_cast<Price>(m_count);
    const Price delta2 = qty - m_mean;
    m_m2 += delta * delta2;

    if (m_count < 2) {
      m_is_block_trade = false;
      return;
    }

    const Price variance = m_m2 / static_cast<Price>(m_count - 1);
    // Guard against zero variance (all identical sizes)
    if (variance <= Price{0}) {
      m_is_block_trade = false;
      return;
    }

    const Price std_dev =
        static_cast<Price>(std::sqrt(static_cast<double>(variance)));
    const Price threshold = m_mean + m_threshold_sigma * std_dev;
    m_is_block_trade = qty > threshold;
  }

  /**
   * @brief Returns true if the most recent trade qualifies as a block print.
   *
   * A block trade is flagged when the executed quantity exceeds the rolling
   * mean by more than N standard deviations (default: 3σ). These prints
   * often represent institutional participation and can precede directional
   * price moves as residual inventory is absorbed.
   */
  std::optional<bool> isBlockTrade() const { return m_is_block_trade; }

  /**
   * @brief Returns the rolling mean trade quantity.
   */
  std::optional<Price> getRollingMeanQuantity() const {
    return m_count > 0 ? std::optional<Price>{m_mean} : std::nullopt;
  }
};
} // namespace signals
