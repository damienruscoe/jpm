#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #119 — Chaikin Oscillator
// L1 | P, Q | Core
// =============================================================================
//
// Applies MACD logic to the A/D line: the difference between a fast EMA and
// a slow EMA of the A/D value. A positive oscillator reading means short-term
// accumulation momentum is outpacing the longer-term trend; negative means
// distribution is accelerating relative to the baseline.
//
template <typename Traits> struct ChaikinOscillator {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  Price m_fast_alpha;
  Price m_slow_alpha;

  // Internal A/D accumulator (mirrors AccumulationDistributionLine logic)
  std::optional<Price> m_last_price{};
  Price m_ad_line{0};
  bool m_has_first_trade{false};

  std::optional<Price> m_ema_fast{};
  std::optional<Price> m_ema_slow{};
  std::optional<Price> m_oscillator{};

  explicit ChaikinOscillator(Price fast_alpha = Price{2} /
                                                Price{10}, // ~fast EMA
                             Price slow_alpha = Price{2} /
                                                Price{28}) // ~slow EMA
      : m_fast_alpha(fast_alpha), m_slow_alpha(slow_alpha) {}

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    const Price money_flow = event.price * qty;
    const Price one = Price{1};

    if (!m_last_price) {
      m_last_price = event.price;
      m_has_first_trade = true;
      return;
    }

    if (event.price > *m_last_price)
      m_ad_line += money_flow;
    else if (event.price < *m_last_price)
      m_ad_line -= money_flow;

    m_last_price = event.price;

    m_ema_fast = (one - m_fast_alpha) * m_ema_fast.value_or(m_ad_line) +
                 m_fast_alpha * m_ad_line;
    m_ema_slow = (one - m_slow_alpha) * m_ema_slow.value_or(m_ad_line) +
                 m_slow_alpha * m_ad_line;
    m_oscillator = *m_ema_fast - *m_ema_slow;
  }

  /**
   * @brief Returns the Chaikin Oscillator value.
   *
   * Positive values indicate that the fast A/D EMA is above the slow A/D EMA,
   * i.e., accumulation momentum is accelerating — a bullish signal. Negative
   * values indicate distribution is accelerating. Crossings of zero are the
   * primary entry/exit triggers. Nullopt until two trades have been seen.
   */
  std::optional<Price> getOscillator() const { return m_oscillator; }
};
} // namespace signals
