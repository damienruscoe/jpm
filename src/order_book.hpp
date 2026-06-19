#pragma once

#include <map>
#include <optional>
#include <vector>

#include "fixed_point.hpp"
#include "ladder.hpp"
#include "object_resource.hpp"

template <typename Level, typename MatchingEnginePolicy> class OrderBook {
public:
  using Price = typename Level::Price;
  using Quantity = typename Level::Quantity;
  using StoredOrderID = typename Level::OrderID;

  template <typename OrderID>
  [[nodiscard]] bool newOrder(OrderID &&order_id, side_t side,
                              const Price &price, const Quantity &quantity);
  template <typename OrderID>
  [[nodiscard]] bool cancel(OrderID &&order_id, side_t side);
  template <typename OrderID>
  [[nodiscard]] bool amend(OrderID &&order_id, side_t side, const Price &price,
                           const Quantity &quantity);

  std::optional<Level> getBest(side_t side) const;
  std::optional<Level> getBestBid() const;
  std::optional<Level> getBestAsk() const;

  std::vector<Level> getTop(side_t side, uint16_t depth = 10) const;
  std::vector<Level> getTopBid(uint16_t depth = 10) const;
  std::vector<Level> getTopAsk(uint16_t depth = 10) const;

private:
  using Order = ::Order<StoredOrderID, Price, Quantity>;
  using MatchingEngine = MatchingEnginePolicy;
  using BidsBook = std::pmr::map<Price, PriceLevel<Order>, std::greater<Price>>;
  using AsksBook = std::pmr::map<Price, PriceLevel<Order>, std::less<Price>>;

  void on_filled(Order &order) { orders.erase(order.id); };

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
template <typename OrderID>
bool OrderBook<Level, MatchingEngine>::newOrder(OrderID &&order_id, side_t side,
                                                const Price &price,
                                                const Quantity &quantity) {
  if (orders.contains(order_id))
    return false;

  auto on_filled = std::bind_front(&OrderBook::on_filled, this);

  auto remaining = side == side_t::BID
                       ? MatchingEngine::matchPrice(asks, price, quantity,
                                                    std::move(on_filled))
                       : MatchingEngine::matchPrice(bids, price, quantity,
                                                    std::move(on_filled));

  if (remaining > 0) {
    auto *order = orders.create(order_id, order_id, price, remaining);
    side == side_t::BID ? MatchingEngine::insertOrder(bids, *order)
                        : MatchingEngine::insertOrder(asks, *order);
  }
  return true;
}

template <typename Level, typename MatchingEngine>
template <typename OrderID>
bool OrderBook<Level, MatchingEngine>::cancel(OrderID &&order_id, side_t side) {
  if (auto *order = orders.find(std::forward<OrderID>(order_id))) {
    side == side_t::BID ? MatchingEngine::removeOrder(bids, *order)
                        : MatchingEngine::removeOrder(asks, *order);
    orders.erase(order->id);
    return true;
  }
  return false;
}

template <typename Level, typename MatchingEngine>
template <typename OrderID>
bool OrderBook<Level, MatchingEngine>::amend(OrderID &&order_id, side_t side,
                                             const Price &price,
                                             const Quantity &quantity) {
  if (auto *order = orders.find(std::forward<OrderID>(order_id))) {
    auto on_filled = std::bind_front(&OrderBook::on_filled, this);

    side == side_t::BID
        ? MatchingEngine::amendOrder(bids, asks, *order, price, quantity,
                                     std::move(on_filled))
        : MatchingEngine::amendOrder(asks, bids, *order, price, quantity,
                                     std::move(on_filled));
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
