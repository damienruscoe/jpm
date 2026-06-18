#pragma once

#include <list>
#include <optional>
#include <variant>
#include <vector>

#include "fixed_point.hpp"
#include "ladder.hpp"
#include "object_resource.hpp"

using OrderID = std::string_view;

template <typename Level, typename MatchingEnginePolicy> class OrderBook {
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
  using MatchingEngine = MatchingEnginePolicy;
  using BidsBook = std::pmr::map<Price, PriceLevel<Order>, std::greater<Price>>;
  using AsksBook = std::pmr::map<Price, PriceLevel<Order>, std::less<Price>>;

  std::pmr::unsynchronized_pool_resource pool;
  BidsBook bids{&pool};
  AsksBook asks{&pool};

  ObjectResource<std::string, Order> orders;
};

namespace utils {
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
} // namespace utils

template <typename Level, typename MatchingEngine>
bool OrderBook<Level, MatchingEngine>::update(OrderID order_id, side_t side,
                                              Level level) {
  if (orders.contains(order_id))
    return false;

  auto quantity = level.quantity;
  side == side_t::BID ? MatchingEngine::matchPrice(asks, level.price, quantity)
                      : MatchingEngine::matchPrice(bids, level.price, quantity);

  if (quantity > 0) {
    auto *order =
        orders.create(order_id, order_id, level.price, quantity, side);
    side == side_t::BID ? MatchingEngine::insertOrder(bids, *order)
                        : MatchingEngine::insertOrder(asks, *order);
  }
  return true;
}

template <typename Level, typename MatchingEngine>
bool OrderBook<Level, MatchingEngine>::cancel(OrderID order_id, side_t side) {
  if (auto *order = orders.find(order_id)) {
    side == side_t::BID
        ? MatchingEngine::removeOrder(bids, *order, order->price)
        : MatchingEngine::removeOrder(asks, *order, order->price);
    orders.erase(*order);
    return true;
  }
  return false;
}

template <typename Level, typename MatchingEngine>
bool OrderBook<Level, MatchingEngine>::amend(OrderID order_id, side_t side,
                                             Level level) {
  if (auto *order = orders.find(order_id)) {
    side == side_t::BID
        ? MatchingEngine::amendOrder(bids, asks, *order, level.price,
                                     level.quantity)
        : MatchingEngine::amendOrder(asks, bids, *order, level.price,
                                     level.quantity);
    return true;
  }
  return false;
}

template <typename Level, typename MatchingEngine>
std::optional<Level>
OrderBook<Level, MatchingEngine>::getBest(side_t side) const {
  return side == side_t::ASK ? getBestAsk() : getBestBid();
}

template <typename Level, typename MatchingEngine>
std::vector<Level>
OrderBook<Level, MatchingEngine>::getTop(side_t side, uint16_t depth) const {
  return side == side_t::ASK ? getTopAsk(depth) : getTopBid(depth);
}

template <typename Level, typename MatchingEngine>
std::optional<Level> OrderBook<Level, MatchingEngine>::getBestAsk() const {
  return utils::getBest<Level>(asks);
}

template <typename Level, typename MatchingEngine>
std::optional<Level> OrderBook<Level, MatchingEngine>::getBestBid() const {
  return utils::getBest<Level>(bids);
}

template <typename Level, typename MatchingEngine>
std::vector<Level>
OrderBook<Level, MatchingEngine>::getTopAsk(uint16_t depth) const {
  return utils::getTop<Level>(asks, depth);
}

template <typename Level, typename MatchingEngine>
std::vector<Level>
OrderBook<Level, MatchingEngine>::getTopBid(uint16_t depth) const {
  return utils::getTop<Level>(bids, depth);
}
