#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
template <typename Traits> struct LastTradePrice {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_last_trade_price{};
  std::optional<Quantity> m_last_trade_quantity{};

  void update(const TradeEvent<Traits> &event) {
    m_last_trade_price = event.price;
    m_last_trade_quantity = event.quantity;
  }

  /**
   * @brief Returns the last executed trade price.
   */
  std::optional<Price> getLastTradedPrice() const { return m_last_trade_price; }

  /**
   * @brief Returns the last executed trade quantity.
   */
  std::optional<Quantity> getLastTradedQuantity() const {
    return m_last_trade_quantity;
  }
};
} // namespace signals
