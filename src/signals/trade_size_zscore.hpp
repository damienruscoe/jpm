#pragma once

#include "trade_event.hpp"
#include <cmath>
#include <optional>

namespace signals {
// =============================================================================
// #83 — Trade-Sized Z-Score
// L1 | Q | Core
// =============================================================================
//
// Maintains a rolling window of trade quantities and computes the Z-score of
// the most recent trade against that window's mean and standard deviation.
// Uses Welford's online algorithm for numerically stable single-pass updates.
//
template <typename Traits> struct TradeSizeZScore {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  int m_count{0};
  Price m_mean{0};
  Price m_m2{0};
  std::optional<Price> m_z_score{};

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);

    m_count++;
    const Price delta = qty - m_mean;
    m_mean += delta / static_cast<Price>(m_count);
    const Price delta2 = qty - m_mean;
    m_m2 += delta * delta2;

    if (m_count < 2) {
      m_z_score = std::nullopt;
      return;
    }

    const Price variance = m_m2 / static_cast<Price>(m_count - 1);
    if (variance <= Price{0}) {
      m_z_score = Price{0};
      return;
    }

    const Price std_dev =
        static_cast<Price>(std::sqrt(static_cast<double>(variance)));
    m_z_score = (qty - m_mean) / std_dev;
  }

  /**
   * @brief Returns the Z-score of the most recent trade quantity.
   *
   * Expresses how many standard deviations the current trade size sits
   * above or below the rolling historical mean. High positive Z-scores
   * flag statistically anomalous large prints; high negative values flag
   * unusually small fragmented fills typical of algorithmic slicing.
   * Nullopt until at least two trades have been observed.
   */
  std::optional<Price> getZScore() const { return m_z_score; }

  /**
   * @brief Returns the rolling mean trade quantity.
   */
  std::optional<Price> getRollingMean() const {
    return m_count > 0 ? std::optional<Price>{m_mean} : std::nullopt;
  }
};
} // namespace signals
