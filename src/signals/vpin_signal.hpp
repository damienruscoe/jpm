#pragma once

#include "trade_event.hpp"
#include <deque>
#include <optional>

namespace signals {
// =============================================================================
// #61 — Volume-Synchronized Probability of Informed Trading (VPIN)
// L1 | P, Q | Core
// =============================================================================
//
// Divides the trade stream into fixed-volume buckets. Within each bucket,
// volume is classified as buy- or sell-initiated by comparing the trade price
// against the prior trade price (tick-rule proxy for aggressor side). VPIN is
// then the average absolute imbalance across the last N buckets, normalized
// by the bucket size. A rising VPIN indicates increasing order flow toxicity.
//
template <typename Traits> struct VpinSignal {
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  std::optional<Price> m_last_price{};
  Price m_buy_volume_bucket{0};
  Price m_sell_volume_bucket{0};
  Price m_bucket_fill{0};
  Price m_bucket_size;
  std::deque<Price> m_imbalances{};
  int m_num_buckets;
  std::optional<Price> m_vpin{};

  explicit VpinSignal(Price bucket_size = Price{50}, int num_buckets = 10)
      : m_bucket_size(bucket_size), m_num_buckets(num_buckets) {}

  void update(const TradeEvent<Traits> &event) {
    const Price qty = static_cast<Price>(event.quantity);

    // Tick-rule aggressor classification
    const bool is_buy = m_last_price ? (event.price >= *m_last_price) : true;
    m_last_price = event.price;

    Price remaining = qty;
    while (remaining > Price{0}) {
      const Price space = m_bucket_size - m_bucket_fill;
      const Price filled = remaining < space ? remaining : space;

      if (is_buy)
        m_buy_volume_bucket += filled;
      else
        m_sell_volume_bucket += filled;

      m_bucket_fill += filled;
      remaining -= filled;

      if (m_bucket_fill >= m_bucket_size) {
        const Price imbalance =
            m_buy_volume_bucket > m_sell_volume_bucket
                ? m_buy_volume_bucket - m_sell_volume_bucket
                : m_sell_volume_bucket - m_buy_volume_bucket;
        m_imbalances.push_back(imbalance / m_bucket_size);
        if (static_cast<int>(m_imbalances.size()) > m_num_buckets)
          m_imbalances.pop_front();

        m_buy_volume_bucket = Price{0};
        m_sell_volume_bucket = Price{0};
        m_bucket_fill = Price{0};

        Price sum{0};
        for (const auto &v : m_imbalances)
          sum += v;
        m_vpin = sum / static_cast<Price>(m_imbalances.size());
      }
    }
  }

  /**
   * @brief Returns the current VPIN estimate.
   *
   * Ranges from 0 to 1. Values approaching 1 indicate severe order flow
   * toxicity — a high fraction of volume is one-directional, implying
   * informed trading pressure. Values near 0 indicate balanced, benign flow.
   * Nullopt until the first complete volume bucket has been processed.
   */
  std::optional<Price> getVpin() const { return m_vpin; }
};
} // namespace signals
