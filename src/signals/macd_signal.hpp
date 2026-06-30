#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #98 — Moving Average Convergence Divergence (MACD)
// L1 | P | Core
// =============================================================================
//
// Description
//
template <typename Traits> struct MacdSignal {
  using Price = typename Traits::Price;

  std::optional<Price> m_ema_fast{};
  std::optional<Price> m_ema_slow{};
  std::optional<Price> m_signal_line{};

  void update(const TradeEvent<Traits> &event) {
    const Price alpha_fast = Price{12} / Price{100};
    const Price alpha_slow = Price{26} / Price{100};
    const Price alpha_sig = Price{9} / Price{100};
    const Price one = Price{1};

    m_ema_fast = (one - alpha_fast) * m_ema_fast.value_or(event.price) +
                 alpha_fast * event.price;
    m_ema_slow = (one - alpha_slow) * m_ema_slow.value_or(event.price) +
                 alpha_slow * event.price;

    if (m_ema_fast && m_ema_slow) {
      Price macd = *m_ema_fast - *m_ema_slow;
      m_signal_line =
          (one - alpha_sig) * m_signal_line.value_or(macd) + alpha_sig * macd;
    }
  }

  std::optional<Price> getMacd() const {
    if (m_ema_fast && m_ema_slow)
      return *m_ema_fast - *m_ema_slow;
    return std::nullopt;
  }

  std::optional<Price> getSignalLine() const { return m_signal_line; }
};
} // namespace signals
