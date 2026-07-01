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

template <typename OrderID_T, typename Price_T, typename Quantity_T>
struct OrderBookTraits {
  using OrderID = OrderID_T;
  using Price = Price_T;
  using Quantity = Quantity_T;
};

template <typename OrderBook, typename SignalAggregator>
struct BaseSignalsDecorator : public signals::BaseSignals<OrderBook>,
                              public SignalAggregator {
  BaseSignalsDecorator(const OrderBook &book)
      : signals::BaseSignals<OrderBook>{book} {}
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
  using Aggregator = BaseSignalsDecorator<OrderBook, SignalAggregator>;

  struct L2PriceLevel {
    Price price;
    Quantity quantity;
    Quantity total;
  };

  OrderBook() : signals(*this) {}

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

  Aggregator &getSignals() { return signals; }
  const Aggregator &getSignals() const { return signals; }

private:
  using BidsBook = OrderBookSide<Order, std::greater<Price>>;
  using AsksBook = OrderBookSide<Order, std::less<Price>>;

  struct CrossingTradeDispatcher {

    template <typename OrderID>
    static Quantity emplaceOrder(OrderBook &order_book, auto &aggressor,
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
        return remaining;
      }
      return {};
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

  void on_filled(side_t aggressor_side, const StoredOrderID &filled_order_id,
                 const Price &price, const Quantity &qty, FillStatus fill) {
    signals.update(
        TradeEvent<Traits>{filled_order_id, price, qty, aggressor_side, fill});

    if (fill == FillStatus::Full)
      orders.erase(filled_order_id);
  };

  Orders orders;
  Aggregator signals;

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

  auto remaining = side == side_t::BID
                       ? CrossingTradeDispatcher::emplaceOrder(
                             *this, asks, bids, std::forward<OrderID>(order_id),
                             price, qty, side)
                       : CrossingTradeDispatcher::emplaceOrder(
                             *this, bids, asks, std::forward<OrderID>(order_id),
                             price, qty, side);

  signals.update(OrderMatchedEvent<Traits>{typename Traits::OrderID{order_id},
                                           price, qty, remaining, side});

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
