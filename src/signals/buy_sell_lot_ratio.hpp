#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
// =============================================================================
// #73 — Aggressive Buy/Sell Lot Ratio
// L1 | Q | Core
// =============================================================================
//
// Maintains separate EMA accumulators for buy-initiated and sell-initiated
// volume. Aggressor direction is determined by tick rule: a trade at or above
// the prior price is classified as a buy; below is a sell. The ratio of the
// two EMAs provides a rolling measure of directional volume skew.
//
template <typename Traits> struct BuySellLotRatio {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  Price m_alpha;
  std::optional<Price> m_last_price{};
  std::optional<Price> m_ema_buy_vol{};
  std::optional<Price> m_ema_sell_vol{};

  explicit BuySellLotRatio(Price alpha = Price{5} / Price{100})
      : m_alpha(alpha) {}

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);
    const Price one = Price{1};
    const bool is_buy = m_last_price ? (event.price >= *m_last_price) : true;
    m_last_price = event.price;

    const Price buy_qty = is_buy ? qty : Price{0};
    const Price sell_qty = !is_buy ? qty : Price{0};

    m_ema_buy_vol =
        (one - m_alpha) * m_ema_buy_vol.value_or(buy_qty) + m_alpha * buy_qty;
    m_ema_sell_vol = (one - m_alpha) * m_ema_sell_vol.value_or(sell_qty) +
                     m_alpha * sell_qty;
  }

  /**
   * @brief Returns the rolling buy/sell lot ratio.
   *
   * Values greater than 1 indicate buy-side volume dominance; values below
   * 1 indicate sell-side dominance. Sustained divergence from 1.0 can signal
   * directional inventory pressure from active participants. Nullopt until
   * sell-side EMA is positive.
   */
  std::optional<Price> getBuySellRatio() const {
    if (m_ema_sell_vol && *m_ema_sell_vol > Price{0})
      return m_ema_buy_vol.value_or(Price{0}) / *m_ema_sell_vol;
    return std::nullopt;
  }

  /**
   * @brief Returns the EMA of buy-initiated volume.
   */
  std::optional<Price> getEmaBuyVolume() const { return m_ema_buy_vol; }

  /**
   * @brief Returns the EMA of sell-initiated volume.
   */
  std::optional<Price> getEmaSellVolume() const { return m_ema_sell_vol; }
};
} // namespace signals
