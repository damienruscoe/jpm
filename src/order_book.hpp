#pragma once

#include "order_book_side.hpp"
#include "order_id.hpp"

#include <memory_resource>
#include <optional>
#include <vector>

#include "object_resource.hpp"

template <typename T, typename ReturnType = decltype(T::id)> struct GetIdField {
  static ReturnType get(const T *t) { return t->id; };
};

template <typename OrderID_t, typename Price_t, typename Quantity_t>
class OrderBook {
public:
  using Price = Price_t;
  using Quantity = Quantity_t;
  using StoredOrderID = OrderID_t;

  struct L2PriceLevel {
    Price price;
    Quantity quantity;
  };

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

  std::optional<Price> getSpread() const {
    auto best_bid = getBestBid();
    auto best_ask = getBestAsk();
    if (best_bid && best_ask)
      return best_ask->price - best_bid->price;
    return std::nullopt;
  }

private:
  using Order = ::Order<StoredOrderID, Price, Quantity>;
  using BidsBook = OrderBookSide<Order, std::greater<Price>>;
  using AsksBook = OrderBookSide<Order, std::less<Price>>;
  using Orders = ObjectResource<Order, GetIdField<Order, StoredOrderID>>;

  struct CrossingTradeDispatcher {

    static auto matchPrice(OrderBook &order_book, auto &aggressor,
                           const Price &price, const Quantity &qty) {
      auto on_filled = std::bind_front(&OrderBook::on_filled, &order_book);
      return aggressor.matchPrice(price, qty, std::move(on_filled));
    }

    template <typename OrderID>
    static void emplaceOrder(OrderBook &order_book, auto &aggressor,
                             auto &resting, OrderID &&order_id,
                             const Price &price, const Quantity &qty,
                             side_t side) {
      if (const auto remaining =
              matchPrice(order_book, aggressor, price, qty)) {
        auto *order = order_book.orders.create(std::forward<OrderID>(order_id),
                                               price, remaining, side);
        resting.insertOrder(*order);
      }
    }

    static bool amend(OrderBook &order_book, auto &aggressor, auto &resting,
                      Order *order, const Price &price, const Quantity &qty) {
      const auto remaining = matchPrice(order_book, aggressor, price, qty);
      resting.amendOrder(*order, price, remaining);
      return true;
    }
  };

  void on_filled(Order &order) { orders.erase(order.id); };

  Orders orders;

  std::pmr::unsynchronized_pool_resource pool;
  BidsBook bids{pool};
  AsksBook asks{pool};
};

template <typename OrderID, typename Price, typename Quantity>
template <typename OrderID_Param>
bool OrderBook<OrderID, Price, Quantity>::newOrder(OrderID_Param &&order_id,
                                                   side_t side,
                                                   const Price &price,
                                                   const Quantity &qty) {

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

template <typename OrderID, typename Price, typename Quantity>
template <typename OrderID_Param>
bool OrderBook<OrderID, Price, Quantity>::cancel(OrderID_Param &&order_id,
                                                 side_t side) {
  if (auto *order = orders.find(std::forward<OrderID_Param>(order_id))) {
    side == side_t::BID ? bids.removeOrder(*order) : asks.removeOrder(*order);
    orders.erase(order->id);
    return true;
  }
  return false;
}

template <typename OrderID, typename Price, typename Quantity>
template <typename OrderID_Param>
bool OrderBook<OrderID, Price, Quantity>::amend(OrderID_Param &&order_id,
                                                side_t side, const Price &price,
                                                const Quantity &qty) {
  if (auto *order = orders.find(std::forward<OrderID_Param>(order_id))) {
    return order->side == side &&
           (side == side_t::BID ? CrossingTradeDispatcher::amend(
                                      *this, asks, bids, order, price, qty)
                                : CrossingTradeDispatcher::amend(
                                      *this, bids, asks, order, price, qty));
  }
  return false;
}
