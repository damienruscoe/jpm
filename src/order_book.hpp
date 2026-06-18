#pragma once

#include <list>
#include <optional>
#include <variant>
#include <vector>

#include "fixed_point.hpp"
#include "ladder.hpp"
#include "object_resource.hpp"

using OrderID = std::string;

template <typename Level> class OrderBook {
public:
  using Price = typename Level::Price;
  using Quantity = typename Level::Quantity;
  using Order = ::Order<Price, Quantity>;

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
  using BidsBook = std::pmr::map<Price, PriceLevel<Order>, std::greater<Price>>;
  using AsksBook = std::pmr::map<Price, PriceLevel<Order>, std::less<Price>>;

  std::pmr::unsynchronized_pool_resource pool;
  BidsBook bids{&pool};
  AsksBook asks{&pool};

  ObjectResource<OrderID, Order> orders;
};

namespace market_side_utils {

template <typename MarketSide, typename OpposingLadder, typename OrderID,
          typename Order, typename Price, typename Quantity>
void match(MarketSide &aggressor, OpposingLadder &opposing,
           ObjectResource<OrderID, Order> &orders, OrderID order_id,
           Price price, Quantity quantity, side_t side) {
  const auto comp = opposing.key_comp();
  const auto level_end = opposing.end();

  auto level = opposing.begin();
  while (quantity > 0 && level != level_end && !comp(price, level->first)) {
    const bool cleared = level->second.matchAgainst(
        quantity, [&](Order &order) { orders.erase(order); });
    level = cleared ? opposing.erase(level) : std::next(level);
  }

  if (quantity > 0) {
    Order *order = orders.create(order_id, order_id, price, quantity, side);
    auto [it, added] = aggressor.try_emplace(price);
    it->second.addOrder(*order);
  }
}

template <typename MarketSide, typename Order, typename Price>
void removeOrder(MarketSide &market_side, Order &order, Price price) {
  const auto it = market_side.find(price);
  if (it != market_side.end() && it->second.removeOrder(order))
    market_side.erase(it);
}

template <typename Level, typename MarketSide>
std::optional<Level> getBest(const MarketSide &market_side) {
  auto it = market_side.begin();
  return it == market_side.end() ? std::nullopt
                                 : Level{it->first, it->second.getQuantity()};
}

template <typename Level, typename MarketSide>
std::vector<Level> getTop(const MarketSide &market_side, uint16_t depth) {
  std::vector<Level> result{};
  for (const auto &level : market_side) {
    if (result.size() >= depth)
      break;
    result.push_back({level.first, level.second.getQuantity()});
  }
  return result;
}

} // namespace market_side_utils

template <typename Level>
bool OrderBook<Level>::update(OrderID order_id, side_t side, Level level) {
  if (orders.contains(order_id))
    return false;

  side == side_t::BID
      ? market_side_utils::match(bids, asks, orders, order_id, level.price,
                                 level.quantity, side)
      : market_side_utils::match(asks, bids, orders, order_id, level.price,
                                 level.quantity, side);
  return true;
}

template <typename Level>
bool OrderBook<Level>::cancel(OrderID order_id, side_t side) {
  if (auto order = orders.find(order_id)) {
    side == side_t::BID
        ? market_side_utils::removeOrder(bids, *order, order->price)
        : market_side_utils::removeOrder(asks, *order, order->price);
    orders.erase(*order);
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
  return market_side_utils::getBest<Level>(asks);
}

template <typename Level>
std::optional<Level> OrderBook<Level>::getBestBid() const {
  return market_side_utils::getBest<Level>(bids);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopAsk(uint16_t depth) const {
  return market_side_utils::getTop<Level>(asks, depth);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopBid(uint16_t depth) const {
  return market_side_utils::getTop<Level>(bids, depth);
}
