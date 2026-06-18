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
  template <typename AggressorMap, typename OpposingMap, typename Order>
  static void insertOrder(AggressorMap &aggressor, OpposingMap &opposing,
                          Order &order) {
    const auto comp = opposing.key_comp();
    const auto level_end = opposing.end();
    auto quantity = order.quantity;

    auto level = opposing.begin();
    while (quantity > 0 && level != level_end &&
           !comp(order.price, level->first)) {
      const bool cleared = level->second.matchAgainst(
          quantity, [&](auto & /*matched_order*/) {});
      level = cleared ? opposing.erase(level) : std::next(level);
    }

    if (quantity > 0) {
      // Update quantity if partial match
      order.quantity = quantity;
      auto [it, added] = aggressor.try_emplace(order.price);
      it->second.addOrder(order);
    }
  }

  template <typename MarketSide, typename Order, typename Price>
  static void removeOrder(MarketSide &market_side, Order &order, Price price) {
    const auto it = market_side.find(price);
    if (it != market_side.end() && it->second.removeOrder(order))
      market_side.erase(it);
  }

  template <typename AggressorMap, typename OpposingMap, typename Order>
  static bool amendOrder(AggressorMap &aggressor, OpposingMap &opposing,
                         Order &order, typename Order::Price new_price,
                         typename Order::Quantity new_quantity) {
    removeOrder(aggressor, order, order.price);

    order.price = new_price;
    order.quantity = new_quantity;

    insertOrder(aggressor, opposing, order);
    return true;
  }
};
