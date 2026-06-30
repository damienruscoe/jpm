#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #117 — Money Flow Index (MFI)
// L1 | P, Q | Core
// =============================================================================
//
// A volume-weighted oscillator analogous to RSI. Each trade's typical price
// (approximated here by the trade price itself, as high/low bars are absent
// at tick level) is multiplied by its quantity to produce raw money flow.
// Positive flow accrues when the price is above the prior trade's price;
// negative flow accrues otherwise. MFI = 100 - 100 / (1 + MF_ratio), where
// MF_ratio is the ratio of positive to negative money flow over the window.
// EMA accumulators replace a fixed lookback window for tick-level streaming.
//
template <typename Traits> struct MoneyFlowIndex {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  Price m_alpha;
  std::optional<Price> m_last_price{};
  std::optional<Price> m_ema_positive_mf{};
  std::optional<Price> m_ema_negative_mf{};
  std::optional<Price> m_mfi{};

  explicit MoneyFlowIndex(Price alpha = Price{5} / Price{100})
      : m_alpha(alpha) {}

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    const Price money_flow = event.price * qty;
    const Price one = Price{1};

    const bool is_positive =
        m_last_price ? (event.price >= *m_last_price) : true;
    m_last_price = event.price;

    const Price pos_flow = is_positive ? money_flow : Price{0};
    const Price neg_flow = is_positive ? Price{0} : money_flow;

    m_ema_positive_mf = (one - m_alpha) * m_ema_positive_mf.value_or(pos_flow) +
                        m_alpha * pos_flow;
    m_ema_negative_mf = (one - m_alpha) * m_ema_negative_mf.value_or(neg_flow) +
                        m_alpha * neg_flow;

    if (m_ema_negative_mf && *m_ema_negative_mf > Price{0}) {
      const Price mf_ratio = *m_ema_positive_mf / *m_ema_negative_mf;
      m_mfi = Price{100} - (Price{100} / (Price{1} + mf_ratio));
    } else if (m_ema_positive_mf && *m_ema_positive_mf > Price{0}) {
      m_mfi = Price{100};
    } else {
      m_mfi = Price{50};
    }
  }

  /**
   * @brief Returns the Money Flow Index in the range [0, 100].
   *
   * Values above 80 suggest overbought conditions — heavy buying pressure
   * relative to selling over the rolling window. Values below 20 indicate
   * oversold conditions driven by sustained sell-side flow. Unlike RSI,
   * MFI incorporates volume, making it more sensitive to large institutional
   * prints that move price without requiring many trades.
   */
  std::optional<Price> getMfi() const { return m_mfi; }
};
} // namespace signals
