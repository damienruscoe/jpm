#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
template <typename Traits> struct RsiSignal {
  using Price = typename Traits::Price;

  std::optional<Price> m_prev_price{};
  std::optional<Price> m_ema_gain{};
  std::optional<Price> m_ema_loss{};

  void update(const TradeEvent<Traits> &event) {
    const Price alpha = Price{1} / Price{14};
    const Price one = Price{1};
    const Price zero = Price{0};

    if (m_prev_price) {
      Price delta = event.price - *m_prev_price;
      Price gain = delta > zero ? delta : zero;
      Price loss = delta < zero ? -delta : zero;

      m_ema_gain = (one - alpha) * m_ema_gain.value_or(zero) + alpha * gain;
      m_ema_loss = (one - alpha) * m_ema_loss.value_or(zero) + alpha * loss;
    }
    m_prev_price = event.price;
  }

  std::optional<Price> getRsi() const {
    if (m_ema_gain && m_ema_loss) {
      if (*m_ema_loss == Price{0})
        return Price{100};
      Price rs = *m_ema_gain / *m_ema_loss;
      return Price{100} - (Price{100} / (Price{1} + rs));
    }
    return std::nullopt;
  }
};
} // namespace signals
