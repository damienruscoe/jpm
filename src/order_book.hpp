#pragma once

#include "order_book_side.hpp"
#include "order_id.hpp"
#include "price_level.hpp"

#include <functional>
#include <memory_resource>
#include <optional>
#include <vector>

#include "formulas.hpp"
#include "object_resource.hpp"

template <typename T, typename ReturnType = decltype(T::id)> struct GetIdField {
  static ReturnType get(const T *t) { return t->id; };
};

template <typename Callable>
static auto topOfBookInvocation(Callable &&fn, const auto &book) {
  auto best_bid = book.getBestBid();
  auto best_ask = book.getBestAsk();
  return (best_bid && best_ask)
             ? std::make_optional(std::invoke(fn, *best_bid, *best_ask))
             : std::nullopt;
}

template <typename OrderID_T, typename Price_T, typename Quantity_T>
struct OrderBookTraits {
  using OrderID = OrderID_T;
  using Price = Price_T;
  using Quantity = Quantity_T;
};

template <typename Traits> class OrderBook {
public:
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;
  using StoredOrderID = typename Traits::OrderID;
  using Order = ::Order<StoredOrderID, Price, Quantity>;
  using Orders = ObjectResource<Order, GetIdField<Order, StoredOrderID>>;

  struct L2PriceLevel {
    bool operator==(const L2PriceLevel &) const = default;

    Price price;
    Quantity quantity;
    Quantity total;
  };

  using TradeCallback = std::function<void(
      side_t, const StoredOrderID &, const Price &, const Quantity &,
      typename PriceLevel<Order>::FillStatus)>;

  void setOnTradeCallback(TradeCallback cb) { on_trade_callback = cb; }

  template <typename OrderID>
  [[nodiscard]] bool newOrder(OrderID &&order_id, side_t side,
                              const Price &price, const Quantity &qty);
  template <typename OrderID>
  [[nodiscard]] bool cancel(OrderID &&order_id, side_t side);
  template <typename OrderID>
  [[nodiscard]] bool amend(OrderID &&order_id, side_t side, const Price &price,
                           const Quantity &qty);

  std::optional<L2PriceLevel> getBestAsk() const {
    return asks.template getBest<L2PriceLevel>();
  }

  std::optional<L2PriceLevel> getBestBid() const {
    return bids.template getBest<L2PriceLevel>();
  }

  std::vector<L2PriceLevel> getTopAsks(uint16_t depth = 10) const {
    return asks.template getTop<L2PriceLevel>(depth);
  }

  std::vector<L2PriceLevel> getTopBids(uint16_t depth = 10) const {
    return bids.template getTop<L2PriceLevel>(depth);
  }

  const Orders &getOrders() const { return orders; }

  std::optional<Price> getSpread() const {
    return topOfBookInvocation(&Formulas<L2PriceLevel>::spread, *this);
  }

  /**
   * @brief Returns the last executed trade price.
   */
  std::optional<Price> getLastTradedPrice() const { return m_last_trade_price; }

  /**
   * @brief Returns the mid-price based on top-of-book (best bid/offer).
   *
   * Intuitively, this represents the "fair" value if you were to
   * instantly exit the position by trading at the market prices.
   */
  std::optional<Price> getMidPrice() const {
    return topOfBookInvocation(&Formulas<L2PriceLevel>::midPrice, *this);
  }

  /**
   * @brief Returns the volume-weighted mid-price.
   *
   * Intuitively, this gives more weight to the side of the book with more
   * depth (volume), acting as a more robust signal of price direction
   * than a simple mid-price.
   */
  std::optional<Price> getWeightedMidPrice() const {
    return topOfBookInvocation(&Formulas<L2PriceLevel>::weightedMidPrice,
                               *this);
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

  /**
   * @brief Returns the session-wide cumulative VWAP.
   *
   * Provides the absolute average price of all volume traded since the
   * start of the session. It serves as an objective anchor point for
   * evaluating execution performance.
   */
  std::optional<Price> getCumulativeVwap() const {
    if (m_cum_volume && *m_cum_volume > 0)
      return m_cum_value.value_or(Price{0}) / *m_cum_volume;
    return std::nullopt;
  }

  /**
   * @brief Returns the session-wide cumulative volume.
   */
  std::optional<Price> getCumulativeVolume() const { return m_cum_volume; }

  /**
   * @brief Returns the session-wide cumulative value.
   */
  std::optional<Price> getCumulativeValue() const { return m_cum_value; }

private:
  using BidsBook = OrderBookSide<Order, std::greater<Price>>;
  using AsksBook = OrderBookSide<Order, std::less<Price>>;

  struct CrossingTradeDispatcher {

    template <typename OrderID>
    static void emplaceOrder(OrderBook &order_book, auto &aggressor,
                             auto &resting, OrderID &&order_id,
                             const Price &price, const Quantity &qty,
                             side_t side) {
      auto on_filled =
          std::bind_front(&OrderBook::on_filled, &order_book, side);
      if (const auto remaining =
              aggressor.matchPrice(price, qty, std::move(on_filled))) {
        auto *order = order_book.orders.create(std::forward<OrderID>(order_id),
                                               price, remaining, side);
        resting.insertOrder(*order);
      }
    }

    static bool amend(OrderBook &order_book, auto &aggressor, auto &resting,
                      Order *order, const Price &price, const Quantity &qty) {
      auto on_filled =
          std::bind_front(&OrderBook::on_filled, &order_book, order->side);
      const auto remaining =
          aggressor.matchPrice(price, qty, std::move(on_filled));

      resting.removeOrder(*order);
      if (remaining > 0) {
        order->price = price;
        order->quantity = remaining;
        resting.insertOrder(*order);
        return false;
      }

      return true;
    }
  };

  void on_filled(side_t side, const StoredOrderID &filled_order_id,
                 const Price &price, const Quantity &qty,
                 typename PriceLevel<Order>::FillStatus fill) {
    m_last_trade_price = price;

    updateEmaMetrics(price, qty);
    updateCumulativeMetrics(price, qty);

    if (fill == PriceLevel<Order>::FillStatus::Full)
      orders.erase(filled_order_id);

    if (on_trade_callback)
      on_trade_callback(side, filled_order_id, price, qty, fill);
  };

  /**
   * @brief Updates EMA-based (rolling) metrics.
   *
   * EMA Price and EMA VWAP act as a proxy for short-term market sentiment.
   * Because they prioritize recent data (via exponential decay), they are
   * highly responsive to immediate volatility and provide a "moving" view of
   * price and volume-weighted activity.
   */
  void updateEmaMetrics(const Price &price, const Quantity &qty) {
    const Price alpha = *Price::Parse("0.05");
    const Price one = *Price::Parse("1.00");

    m_ema_price = (one - alpha) * m_ema_price.value_or(price) + alpha * price;
    m_ema_pv =
        (one - alpha) * m_ema_pv.value_or(price * qty) + alpha * (price * qty);
    m_ema_v = (one - alpha) * m_ema_v.value_or(static_cast<Price>(qty)) +
              alpha * (static_cast<Price>(qty));
  }

  /**
   * @brief Updates session-wide cumulative metrics.
   *
   * Cumulative VWAP provides an objective measure of the average price
   * paid for the asset throughout the entire trading session. It is
   * highly stable, making it useful for performance benchmarking and
   * historical reporting, as it is unaffected by short-term volatility.
   */
  void updateCumulativeMetrics(const Price &price, const Quantity &qty) {
    m_cum_value = m_cum_value.value_or(Price{0}) + (price * qty);
    m_cum_volume = m_cum_volume.value_or(Price{0}) + static_cast<Price>(qty);
  }

  Orders orders;
  TradeCallback on_trade_callback;

  std::pmr::unsynchronized_pool_resource pool;
  BidsBook bids{pool};
  AsksBook asks{pool};

  std::optional<Price> m_last_trade_price{};
  std::optional<Price> m_ema_price{};
  std::optional<Price> m_ema_pv{};
  std::optional<Price> m_ema_v{};
  std::optional<Price> m_cum_value{};
  std::optional<Price> m_cum_volume{};
};

template <typename Traits>
template <typename OrderID_Param>
bool OrderBook<Traits>::newOrder(OrderID_Param &&order_id, side_t side,
                                 const Price &price, const Quantity &qty) {

  if (orders.contains(order_id))
    return false;

  side == side_t::BID
      ? CrossingTradeDispatcher::emplaceOrder(
            *this, asks, bids, std::forward<OrderID_Param>(order_id), price,
            qty, side)
      : CrossingTradeDispatcher::emplaceOrder(
            *this, bids, asks, std::forward<OrderID_Param>(order_id), price,
            qty, side);

  return true;
}

template <typename Traits>
template <typename OrderID_Param>
bool OrderBook<Traits>::cancel(OrderID_Param &&order_id, side_t side) {
  if (auto *order = orders.find(std::forward<OrderID_Param>(order_id))) {
    side == side_t::BID ? bids.removeOrder(*order) : asks.removeOrder(*order);
    orders.erase(order->id);
    return true;
  }
  return false;
}

template <typename Traits>
template <typename OrderID_Param>
bool OrderBook<Traits>::amend(OrderID_Param &&order_id, side_t side,
                              const Price &price, const Quantity &qty) {
  if (auto *order = orders.find(std::forward<OrderID_Param>(order_id))) {
    if (order->side != side)
      return false;

    if (qty < order->quantity && price == order->price) {
      // Reducing quantity only. Maintain PriceTime priority.
      // This action cannot create any trades against resting.
      // Updating the order quanity is the only action that needs to be taken.
      order->quantity = qty;
      return true;
    }

    const auto filled = side == side_t::BID
                            ? CrossingTradeDispatcher::amend(*this, asks, bids,
                                                             order, price, qty)
                            : CrossingTradeDispatcher::amend(*this, bids, asks,
                                                             order, price, qty);

    if (filled)
      orders.erase(order->id);

    return true;
  }
  return false;
}
