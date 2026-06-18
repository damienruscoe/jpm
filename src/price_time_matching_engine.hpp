#pragma once

#include <algorithm>
#include <concepts>
#include <iterator>
#include <optional>
#include <vector>

template <typename T>
concept LadderStorage = requires(T m) {
  typename T::key_type;
  typename T::mapped_type;
  {
    m.key_comp()
    } -> std::invocable<typename T::key_type, typename T::key_type>;
  {m.begin()};
  {m.end()};
  {m.erase(m.begin())};
  {m.try_emplace(typename T::key_type{})};
  {m.find(typename T::key_type{})};
};

template <typename T, typename Level, typename Ladder, typename Opposing,
          typename Order>
concept MatchingEngine = requires(Ladder &ladder, Opposing &opposing,
                                  Order &order) {
  {T::insertOrder(ladder, opposing, order.id, order.price, order.quantity,
                  order.side, nullptr)};
  {T::removeOrder(ladder, order, order.price)};
  { T::template getBest<Level>(ladder) } -> std::same_as<std::optional<Level>>;
  { T::template getTop<Level>(ladder, 0) } -> std::same_as<std::vector<Level>>;
};

class PriceTimeMatchingEngine {
public:
  template <typename AggressorMap, typename OpposingMap, typename Resource,
            typename Price, typename Quantity>
  static void insertOrder(AggressorMap &aggressor, OpposingMap &opposing,
                          Resource &orders,
                          typename Resource::key_type order_id, Price price,
                          Quantity quantity, side_t side) {
    const auto comp = opposing.key_comp();
    const auto level_end = opposing.end();

    auto level = opposing.begin();
    while (quantity > 0 && level != level_end && !comp(price, level->first)) {
      const bool cleared = level->second.matchAgainst(
          quantity, [&](auto &order) { orders.erase(order); });
      level = cleared ? opposing.erase(level) : std::next(level);
    }

    if (quantity > 0) {
      auto *order = orders.create(order_id, order_id, price, quantity, side);
      auto [it, added] = aggressor.try_emplace(price);
      it->second.addOrder(*order);
    }
  }

  template <typename MarketSide, typename Order, typename Price>
  static void removeOrder(MarketSide &market_side, Order &order, Price price) {
    const auto it = market_side.find(price);
    if (it != market_side.end() && it->second.removeOrder(order))
      market_side.erase(it);
  }

  template <typename AggressorMap, typename OpposingMap, typename Resource,
            typename Price, typename Quantity>
  static bool amendOrder(AggressorMap &aggressor, OpposingMap &opposing,
                         Resource &orders, typename Resource::key_type order_id,
                         typename Resource::value_type &order, Price price,
                         Quantity quantity, side_t side) {
    removeOrder(aggressor, order, order.price);
    orders.erase(order);
    insertOrder(aggressor, opposing, orders, order_id, price, quantity, side);
    return true;
  }
};
