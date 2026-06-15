#pragma once

#include <list>
#include <unordered_map>
#include <vector>

#include "fixed_point.hpp"
#include "ladder.hpp"

enum class side_t { BID, ASK };

using OrderID = uint64_t;

template <typename Level> class OrderBook {
public:
  using Price = typename Level::Price;
  using Quantity = typename Level::Quantity;

  [[nodiscard]] bool update(OrderID order_id, side_t side, Level level);

  Level getBest(side_t side) const;
  Level getBestBid() const;
  Level getBestAsk() const;

  std::vector<Level> getTop(side_t side, uint16_t depth = 10) const;
  std::vector<Level> getTopBid(uint16_t depth = 10) const;
  std::vector<Level> getTopAsk(uint16_t depth = 10) const;

private:
  using Bids = ladder::Ladder<Price, Quantity, std::greater<Price>>;
  using Asks = ladder::Ladder<Price, Quantity, std::less<Price>>;

  using OrderLink = typename std::list<
      typename ladder::Orders<Quantity>::OrderQuantity>::iterator;
  using Orders = std::unordered_map<OrderID, OrderLink>;

  Bids bids;
  Asks asks;
  Orders orders;
};

template <typename ActiveLadder, typename OpposingLadder, typename Orders,
          typename Level>
void update(OrderID order_id, Orders &orders, ActiveLadder &active,
            OpposingLadder &opposing, Level &level) {
  const auto price_exceeded = active.key_comp();
  auto &remaining = level.quantity;
  auto price_level = opposing.begin();
  const auto last_price_level = opposing.end();

  while (remaining && price_level != last_price_level &&
         !price_exceeded(price_level->first, level.price)) {
    auto &level_orders = price_level->second.orders;

    const auto last_order = level_orders.end();
    for (auto order = level_orders.begin(); remaining && order != last_order;) {
      if (order->quantity <= remaining) {
        remaining -= order->quantity;

        orders.erase(order->order_id);

        order = level_orders.erase(order);
        price_level = level_orders.empty() ? opposing.erase(price_level)
                                           : std::next(price_level);
      } else {
        order->quantity -= remaining;
        return;
      }
    }
  }

  if (remaining != 0)
    orders.emplace(order_id,
                   active.addOrder(order_id, level.price, level.quantity));
}

template <typename Level>
bool OrderBook<Level>::update(OrderID order_id, side_t side, Level level) {
  if (orders.find(order_id) != orders.end())
    return false;

  side == side_t::ASK ? ::update(order_id, orders, asks, bids, level)
                      : ::update(order_id, orders, bids, asks, level);

  return true;
}

template <typename Level> Level OrderBook<Level>::getBest(side_t side) const {
  return side == side_t::ASK ? getBestAsk() : getBestBid();
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTop(side_t side, uint16_t depth) const {
  return side == side_t::ASK ? getTopAsk(depth) : getTopBid(depth);
}

template <typename Level> Level OrderBook<Level>::getBestAsk() const {
  return ladder::getBest<Asks, Level>(asks);
}

template <typename Level> Level OrderBook<Level>::getBestBid() const {
  return ladder::getBest<Bids, Level>(bids);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopAsk(uint16_t depth) const {
  return ladder::getTop<Asks, Level>(asks, depth);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopBid(uint16_t depth) const {
  return ladder::getTop<Bids, Level>(bids, depth);
}
