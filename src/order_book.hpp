#pragma once

#include <map>
#include <optional>
#include <vector>

#include "fixed_point.hpp"
#include "ladder.hpp"
#include "object_resource.hpp"

template <typename Order, typename Comparitor> class MarketSide {
public:
  using Price = typename Order::Price;
  using Quantity = typename Order::Quantity;

  MarketSide(std::pmr::unsynchronized_pool_resource &pool) : book(&pool) {}

  template <typename OrderCallback>
  Quantity matchPrice(const Price &price, Quantity qty,
                      OrderCallback &&on_filled) {
    const auto comp = typename Book::key_compare{};
    const auto level_end = book.end();

    auto level = book.begin();
    while (qty > 0 && level != level_end && !comp(price, level->first)) {
      const bool cleared = level->second.matchAgainst(
          qty, std::forward<OrderCallback>(on_filled));
      level = cleared ? book.erase(level) : std::next(level);
    }

    return qty;
  }

  void insertOrder(Order &order) { book[order.price].addOrder(order); }

  void removeOrder(Order &order) {
    const auto it = book.find(order.price);
    if (it != book.end() && it->second.removeOrder(order))
      book.erase(it);
  }

  void amendOrder(Order &order, Price price, Quantity qty) {
    removeOrder(order);
    if (qty > 0) {
      order.price = price;
      order.quantity = qty;
      insertOrder(order);
    }
  }

  template <typename Level> std::optional<Level> getBest() const {
    auto it = book.begin();
    return it == book.end() ? std::nullopt
                            : Level{it->first, it->second.getQuantity()};
  }

  template <typename Level> std::vector<Level> getTop(uint16_t depth) const {
    std::vector<Level> result{};
    for (const auto &[price, price_orders] : book) {
      if (result.size() >= depth)
        break;
      result.push_back({price, price_orders.getQuantity()});
    }
    return result;
  }

private:
  using Book = std::pmr::map<Price, PriceLevel<Order>, Comparitor>;
  Book book;
};

template <typename Price, typename Quantity> struct CrossingTradeDispatcher {
  template <typename OrderID, typename OrderBook, typename AggressorSide,
            typename RestingSide>
  static void newOrder(OrderBook &order_book, AggressorSide &aggressor,
                       RestingSide &resting, OrderID &&order_id,
                       const Price &price, const Quantity &qty) {
    const auto on_filled = std::bind_front(&OrderBook::on_filled, &order_book);

    if (const auto remaining =
            aggressor.matchPrice(price, qty, std::move(on_filled))) {
      auto *order =
          order_book.orders.create(order_id, order_id, price, remaining);
      resting.insertOrder(*order);
    }
  }

  template <typename Order, typename OrderBook, typename AggressorSide,
            typename RestingSide>
  static void amend(OrderBook &order_book, AggressorSide &aggressor,
                    RestingSide &resting, Order *order, const Price &price,
                    const Quantity &qty) {
    const auto on_filled = std::bind_front(&OrderBook::on_filled, &order_book);

    const auto remaining =
        aggressor.matchPrice(price, qty, std::move(on_filled));
    resting.amendOrder(*order, price, remaining);
  }
};

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
  using CrossingTradeDispatcher = ::CrossingTradeDispatcher<Price, Quantity>;
  friend CrossingTradeDispatcher;

  using Order = ::Order<StoredOrderID, Price, Quantity>;
  using BidsBook = MarketSide<Order, std::greater<Price>>;
  using AsksBook = MarketSide<Order, std::less<Price>>;

  void on_filled(Order &order) { orders.erase(order.id); };

  std::pmr::unsynchronized_pool_resource pool;
  BidsBook bids{pool};
  AsksBook asks{pool};

  ObjectResource<std::string, Order> orders;
};

template <typename Level>
template <typename OrderID>
bool OrderBook<Level>::newOrder(OrderID &&order_id, side_t side,
                                const Price &price, const Quantity &qty) {

  if (orders.contains(order_id))
    return false;

  side == side_t::BID
      ? CrossingTradeDispatcher::newOrder(
            *this, asks, bids, std::forward<OrderID>(order_id), price, qty)
      : CrossingTradeDispatcher::newOrder(
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
