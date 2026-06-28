#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
template <typename Traits> struct EmaSignal {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_ema_price{};
  std::optional<Price> m_ema_pv{};
  std::optional<Price> m_ema_v{};

  void update(const TradeEvent<Traits> &event) {
    const Price alpha = Price{5} / Price{100};
    const Price one = Price{1};

    const auto &price = event.price;
    const auto &quantity = event.quantity;

    m_ema_price = (one - alpha) * m_ema_price.value_or(price) + alpha * price;
    m_ema_pv = (one - alpha) * m_ema_pv.value_or(price * quantity) +
               alpha * (price * quantity);
    m_ema_v = (one - alpha) * m_ema_v.value_or(static_cast<Price>(quantity)) +
              alpha * (static_cast<Price>(quantity));
  }

  /**
   * @brief Returns the EMA-based (rolling) price.
   *
   * Acts as a responsive "moving average" of trade prices. Because it
   * prioritizes recent activity, it helps smooth out transient noise
   * to highlight the current price trend.
   *
   * @note This is a trade-count-based proxy for TWAP. Prefer this over
   * timestamped TWAP in systems where time-interval data is absent or
   * unreliable, as it avoids time-drift inaccuracies.
   */
  std::optional<Price> getEmaPrice() const { return m_ema_price; }

  /**
   * @brief Returns the EMA-based (rolling) VWAP.
   *
   * Provides a rolling measure of the average price, weighted by trade
   * volume. It helps identify if volume-heavy trades are occurring
   * above or below the EMA price, indicating aggressive buying/selling
   * pressure.
   *
   * @note Unlike cumulative VWAP, this is rolling and highly responsive to
   * immediate volatility. Prefer this for high-frequency signal generation
   * rather than session-wide performance reporting.
   */
  std::optional<Price> getEmaVwap() const {
    if (m_ema_v && *m_ema_v > 0)
      return m_ema_pv.value_or(Price{0}) / *m_ema_v;
    return std::nullopt;
  }
};
} // namespace signals
