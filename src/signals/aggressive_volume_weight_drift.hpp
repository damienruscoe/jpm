#pragma once

#include "trade_event.hpp"
#include <optional>

namespace signals {
template <typename Traits> struct AggressiveVolumeWeightedDrift {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_prev_price{};
  std::optional<Price> m_drift{};

  void update(const TradeEvent<Traits> &event) {
    if (m_prev_price) {
      Price delta = event.price - *m_prev_price;
      m_drift = m_drift.value_or(Price{0}) +
                (delta * static_cast<Price>(event.quantity));
    }
    m_prev_price = event.price;
  }

  std::optional<Price> getVolumeWeightedDrift() const { return m_drift; }
};
} // namespace signals
