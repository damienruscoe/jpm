#pragma once

#include "order_book_side.hpp"

#include <memory_resource>
#include <optional>
#include <vector>

#include "fixed_point.hpp"
#include "object_resource.hpp"

template <typename Level> class OrderBook {
public:
  using Price = typename Level::Price;
  using Quantity = typename Level::Quantity;
  using StoredOrderID = typename Level::OrderID;

  template <typename OrderID>
  [[nodiscard]] bool newOrder(OrderID &&order_id, side_t side,
                              const Price &price, const Quantity &qty);
  template <typename OrderID>
  [[nodiscard]] bool cancel(OrderID &&order_id, side_t side);
  template <typename OrderID>
  [[nodiscard]] bool amend(OrderID &&order_id, side_t side, const Price &price,
                           const Quantity &qty);

  std::optional<Level> getBest(side_t side) const;
  std::optional<Level> getBestBid() const;
  std::optional<Level> getBestAsk() const;

  std::vector<Level> getTop(side_t side, uint16_t depth = 10) const;
  std::vector<Level> getTopBid(uint16_t depth = 10) const;
  std::vector<Level> getTopAsk(uint16_t depth = 10) const;

private:
  using Order = ::Order<StoredOrderID, Price, Quantity>;
  using BidsBook = OrderBookSide<Order, std::greater<Price>>;
  using AsksBook = OrderBookSide<Order, std::less<Price>>;
  using Orders = ObjectResource<StoredOrderID, Order>;

  struct CrossingTradeDispatcher {
    template <typename AggressorSide>
    static auto matchPrice(OrderBook &order_book, AggressorSide &aggressor,
                           const Price &price, const Quantity &qty) {
      const auto on_filled =
          std::bind_front(&OrderBook::on_filled, &order_book);
      return aggressor.matchPrice(price, qty, std::move(on_filled));
    }

    template <typename OrderID, typename AggressorSide, typename RestingSide>
    static void emplaceOrder(OrderBook &order_book, AggressorSide &aggressor,
                             RestingSide &resting, OrderID &&order_id,
                             const Price &price, const Quantity &qty) {
      if (const auto remaining =
              matchPrice(order_book, aggressor, price, qty)) {
        auto *order =
            order_book.orders.create(order_id, order_id, price, remaining);
        resting.insertOrder(*order);
      }
    }

    template <typename AggressorSide, typename RestingSide>
    static void amend(OrderBook &order_book, AggressorSide &aggressor,
                      RestingSide &resting, Order *order, const Price &price,
                      const Quantity &qty) {
      const auto remaining = matchPrice(order_book, aggressor, price, qty);
      resting.amendOrder(*order, price, remaining);
    }
  };

  void on_filled(Order &order) { orders.erase(order.id); };

  std::pmr::unsynchronized_pool_resource pool;
  BidsBook bids{pool};
  AsksBook asks{pool};

  Orders orders;
};

template <typename Level>
template <typename OrderID>
bool OrderBook<Level>::newOrder(OrderID &&order_id, side_t side,
                                const Price &price, const Quantity &qty) {

  if (orders.contains(order_id))
    return false;

  side == side_t::BID
      ? CrossingTradeDispatcher::emplaceOrder(
            *this, asks, bids, std::forward<OrderID>(order_id), price, qty)
      : CrossingTradeDispatcher::emplaceOrder(
            *this, bids, asks, std::forward<OrderID>(order_id), price, qty);

  return true;
}

template <typename Level>
template <typename OrderID>
bool OrderBook<Level>::cancel(OrderID &&order_id, side_t side) {
  if (auto *order = orders.find(std::forward<OrderID>(order_id))) {
    side == side_t::BID ? bids.removeOrder(*order) : asks.removeOrder(*order);
    orders.erase(order->id);
    return true;
  }
  return false;
}

template <typename Level>
template <typename OrderID>
bool OrderBook<Level>::amend(OrderID &&order_id, side_t side,
                             const Price &price, const Quantity &qty) {
  if (auto *order = orders.find(std::forward<OrderID>(order_id))) {
    side == side_t::BID
        ? CrossingTradeDispatcher::amend(*this, asks, bids, order, price, qty)
        : CrossingTradeDispatcher::amend(*this, bids, asks, order, price, qty);

    return true;
  }
  return false;
}

template <typename Level>
std::optional<Level> OrderBook<Level>::getBest(side_t side) const {
  return side == side_t::ASK ? getBestAsk() : getBestBid();
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTop(side_t side, uint16_t depth) const {
  return side == side_t::ASK ? getTopAsk(depth) : getTopBid(depth);
}

template <typename Level>
std::optional<Level> OrderBook<Level>::getBestAsk() const {
  return asks.template getBest<Level>();
}

template <typename Level>
std::optional<Level> OrderBook<Level>::getBestBid() const {
  return bids.template getBest<Level>();
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopAsk(uint16_t depth) const {
  return asks.template getTop<Level>(depth);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopBid(uint16_t depth) const {
  return bids.template getTop<Level>(depth);
}
