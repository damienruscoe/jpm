#pragma once

#include "order_book_side.hpp"
#include "order_id.hpp"
#include "price_level.hpp"

#include <functional>
#include <memory_resource>
#include <optional>
#include <vector>

#include "core/trade_event.hpp"
#include "formulas.hpp"
#include "object_resource.hpp"
#include "signals/signals_base.hpp"

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

template <typename Traits,
          typename SignalAggregator = signals::EmptySignals<Traits>>
class OrderBook {
public:
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;
  using StoredOrderID = typename Traits::OrderID;
  using Order = ::Order<StoredOrderID, Price, Quantity>;
  using Orders = ObjectResource<Order, GetIdField<Order, StoredOrderID>>;
  using Aggregator = SignalAggregator;

  struct L2PriceLevel {
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

  /**
   * @brief Returns the spread of the best bid and best ask price.
   */
  std::optional<Price> getSpread() const {
    return topOfBookInvocation(&Formulas<L2PriceLevel>::spread, *this);
  }

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
   * @brief Returns the micro-price derived from the inside bid/ask imbalance.
   *
   * The micro-price weights the best bid and best ask by the opposing side's
   * quantity, pulling the result toward the thinner side of the book. When
   * bid depth significantly exceeds ask depth the micro-price migrates toward
   * the ask — reflecting the higher probability that the next aggressive trade
   * will lift the offer. It is consistently a better predictor of the next
   * trade price than the simple mid-price.
   *
   * @note Algebraically equivalent to getWeightedMidPrice() but derived from
   * a distinct conceptual route (quantity imbalance ratio rather than
   * opposing-side weighting). Both are retained as separate signals because
   * they are catalogued and used independently in institutional signal
   * libraries.
   */
  std::optional<Price> getMicroPrice() const {
    return topOfBookInvocation(&Formulas<L2PriceLevel>::microPrice, *this);
  }

  /**
   * @brief Returns the normalised quantity imbalance at the inside touch.
   *
   *  ── #36 — Inside Level Volumetric Imbalance ──────────────────────────────
   *
   * Computed as (bid_qty - ask_qty) / (bid_qty + ask_qty), ranging from
   * -1 to +1. A strongly positive reading indicates that passive buy-side
   * resting depth dwarfs the offer — a bullish queue structure. A strongly
   * negative reading indicates offer-side dominance. The signal is a fast,
   * stateless proxy for short-term directional pressure and is frequently
   * used as a feature in short-horizon price prediction models alongside the
   * micro-price and spread.
   *
   * @note Unlike the micro-price (which outputs a price level), this signal
   * outputs a dimensionless ratio and should not be used as a price anchor.
   */
  std::optional<Price> getInsideVolumetricImbalance() const {
    return topOfBookInvocation(
        &Formulas<L2PriceLevel>::insideVolumetricImbalance, *this);
  }

  /**
   * @brief Returns a single-snapshot directional-pressure proxy for CVD.
   *
   * ── #49 — Cumulative Volume Delta (snapshot proxy) ───────────────────────
   *
   * @note This is NOT a true Cumulative Volume Delta. CVD is canonically a
   * running sum of signed *trade* volume over time (see
   * CumulativeVolumeDelta in trade_signals.hpp, which implements the actual
   * cumulative metric from TradeEvent prints). BaseSignals is stateless and
   * has no access to the trade stream — only to the current book snapshot —
   * so a true cumulative figure cannot be produced here.
   *
   * What this method returns instead is the signed inside-touch quantity
   * differential, (bid_qty - ask_qty), expressed in raw quantity units
   * rather than the normalised ratio used by getInsideVolumetricImbalance().
   * It is the instantaneous, non-cumulative analogue: a positive value
   * indicates more resting bid quantity than ask quantity at this instant,
   * which is directionally consistent with (but not equivalent to) a
   * positive CVD reading. Use this only as a same-tick proxy; for genuine
   * session-level buy/sell pressure tracking, use the trade-based
   * CumulativeVolumeDelta signal instead.
   */
  std::optional<Quantity> getVolumeDeltaSnapshot() const {
    return topOfBookInvocation(&Formulas<L2PriceLevel>::volumeDeltaSnapshot,
                               *this);
  }

  SignalAggregator &getSignals() { return signals; }
  const SignalAggregator &getSignals() const { return signals; }

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
    signals.update({price, qty});

    if (fill == PriceLevel<Order>::FillStatus::Full)
      orders.erase(filled_order_id);

    if (on_trade_callback)
      on_trade_callback(side, filled_order_id, price, qty, fill);
  };

  Orders orders;
  SignalAggregator signals;
  TradeCallback on_trade_callback;

  std::pmr::unsynchronized_pool_resource pool;
  BidsBook bids{pool};
  AsksBook asks{pool};
};

template <typename Traits, typename SignalAggregator>
template <typename OrderID>
bool OrderBook<Traits, SignalAggregator>::newOrder(OrderID &&order_id,
                                                   side_t side,
                                                   const Price &price,
                                                   const Quantity &qty) {
  if (orders.contains(order_id))
    return false;

  side == side_t::BID
      ? CrossingTradeDispatcher::emplaceOrder(*this, asks, bids,
                                              std::forward<OrderID>(order_id),
                                              price, qty, side)
      : CrossingTradeDispatcher::emplaceOrder(*this, bids, asks,
                                              std::forward<OrderID>(order_id),
                                              price, qty, side);

  return true;
}

template <typename Traits, typename SignalAggregator>
template <typename OrderID>
bool OrderBook<Traits, SignalAggregator>::cancel(OrderID &&order_id,
                                                 side_t side) {
  if (auto *order = orders.find(std::forward<OrderID>(order_id))) {
    side == side_t::BID ? bids.removeOrder(*order) : asks.removeOrder(*order);
    orders.erase(order->id);
    return true;
  }
  return false;
}

template <typename Traits, typename SignalAggregator>
template <typename OrderID>
bool OrderBook<Traits, SignalAggregator>::amend(OrderID &&order_id, side_t side,
                                                const Price &price,
                                                const Quantity &qty) {
  if (auto *order = orders.find(std::forward<OrderID>(order_id))) {
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
