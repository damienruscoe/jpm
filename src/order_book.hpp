#pragma once

#include <list>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "fixed_point.hpp"
#include "ladder.hpp"
#include "order.hpp"
#include "order_storage.hpp"

using OrderID = std::string;

template <typename Level> class OrderBook {
public:
  using Price = typename Level::Price;
  using Quantity = typename Level::Quantity;
  using Order = ::Order<Price, Quantity>;
  using Bids = Ladder<Price, Quantity, std::greater<Price>>;
  using Asks = Ladder<Price, Quantity, std::less<Price>>;

  [[nodiscard]] bool update(OrderID order_id, side_t side, Level level);
  [[nodiscard]] bool cancel(OrderID order_id, side_t side);
  [[nodiscard]] bool amend(OrderID order_id, side_t side, Level level);

  std::optional<Level> getBest(side_t side) const;
  std::optional<Level> getBestBid() const;
  std::optional<Level> getBestAsk() const;

  std::vector<Level> getTop(side_t side, uint16_t depth = 10) const;
  std::vector<Level> getTopBid(uint16_t depth = 10) const;
  std::vector<Level> getTopAsk(uint16_t depth = 10) const;

private:
  template <typename LadderType, typename OpposingLadder>
  void addToLadder(OrderID order_id, Price price, Quantity quantity,
                   side_t side, LadderType &ladder, OpposingLadder &opposing);

  OrderStorage<Order> storage;

  std::unordered_map<OrderID, Order *> id_to_order_ptr;

  Bids bids;
  Asks asks;
};

template <typename Level>
template <typename LadderType, typename OpposingLadder>
void OrderBook<Level>::addToLadder(OrderID order_id, Price price,
                                   Quantity quantity, side_t side,
                                   LadderType &ladder,
                                   OpposingLadder &opposing) {
  auto on_filled = [this](Order &order) {
    id_to_order_ptr.erase(order.id);
    storage.destroyOrder(&order);
  };

  opposing.matchAgainst(quantity, price, on_filled);
  if (quantity > 0) {
    Order *order = storage.createOrder(order_id, price, quantity, side);
    id_to_order_ptr[order_id] = order;
    ladder.addOrder(order, price, quantity);
  }
}

template <typename Level>
bool OrderBook<Level>::update(OrderID order_id, side_t side, Level level) {
  if (id_to_order_ptr.find(order_id) != id_to_order_ptr.end())
    return false;

  side == side_t::BID
      ? addToLadder(order_id, level.price, level.quantity, side, bids, asks)
      : addToLadder(order_id, level.price, level.quantity, side, asks, bids);
  return true;
}

template <typename Level>
bool OrderBook<Level>::cancel(OrderID order_id, side_t side) {
  if (auto it = id_to_order_ptr.find(order_id); it != id_to_order_ptr.end()) {
    Order &order = *it->second;
    side == side_t::BID ? bids.removeOrder(order) : asks.removeOrder(order);

    id_to_order_ptr.erase(order_id);
    storage.destroyOrder(&order);
    return true;
  }

  return false;
}

template <typename Level>
bool OrderBook<Level>::amend(OrderID order_id, side_t side, Level level) {
  if (cancel(order_id, side)) {
    return update(order_id, side, level);
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
