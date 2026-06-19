#pragma once

#include <algorithm>
#include <concepts>
#include <iterator>
#include <optional>
#include <vector>

class PriceTimeMatchingEngine {
public:
  template <typename MarketSide, typename Price, typename Quantity,
            typename OrderCallback>
  static void matchPrice(MarketSide &market_side, Price price,
                         Quantity &quantity, OrderCallback &&on_filled) {
    const auto comp = typename MarketSide::key_compare{};
    const auto level_end = market_side.end();

    auto level = market_side.begin();
    while (quantity > 0 && level != level_end && !comp(price, level->first)) {
      const bool cleared = level->second.matchAgainst(
          quantity, std::forward<OrderCallback>(on_filled));
      level = cleared ? market_side.erase(level) : std::next(level);
    }
  }

  template <typename AggressorMap, typename Order>
  static void insertOrder(AggressorMap &market_side, Order &order) {
    market_side[order.price].addOrder(order);
  }

  template <typename MarketSide, typename Order>
  static void removeOrder(MarketSide &market_side, Order &order,
                          typename Order::Price price) {
    const auto it = market_side.find(price);
    if (it != market_side.end() && it->second.removeOrder(order))
      market_side.erase(it);
  }

  template <typename AggressorMap, typename OpposingMap, typename Order,
            typename OrderCallback>
  static void amendOrder(AggressorMap &aggressor, OpposingMap &opposing,
                         Order &order, typename Order::Price new_price,
                         typename Order::Quantity new_quantity,
                         OrderCallback &&on_filled) {
    matchPrice(opposing, new_price, new_quantity,
               std::forward<OrderCallback>(on_filled));

    removeOrder(aggressor, order, order.price);
    if (new_quantity > 0) {
      order.price = new_price;
      order.quantity = new_quantity;

      insertOrder(aggressor, order);
    }
  }
};
