#pragma once

#include <boost/intrusive/list.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

enum class side_t { BID, ASK };
using OrderID = std::string;

template <typename PriceT, typename QuantityT> struct Order {
  using Price = PriceT;
  using Quantity = QuantityT;

  OrderID id;
  Price price;
  Quantity quantity;
  side_t side;
  boost::intrusive::list_member_hook<> ladder_hook;

  Order(OrderID _id, Price _price, Quantity _quantity, side_t _side)
      : id(std::move(_id)), price(_price), quantity(_quantity), side(_side) {}
};

template <typename T>
concept LadderOrder = requires(T o) {
  typename T::Price;
  typename T::Quantity;
  { o.quantity } -> std::convertible_to<typename T::Quantity>;
  {o.ladder_hook};
};

template <LadderOrder Order> struct PriceLevel {
private:
  using Quantity = typename Order::Quantity;

  using OrderList = boost::intrusive::list<
      Order,
      boost::intrusive::member_hook<Order, boost::intrusive::list_member_hook<>,
                                    &Order::ladder_hook>>;

public:
  void addOrder(Order &order) {
    orders.push_back(order);
    total_quantity += order.quantity;
  }

  [[nodiscard]] bool removeOrder(Order &order) {
    total_quantity -= order.quantity;
    orders.erase(orders.iterator_to(order));
    return orders.empty();
  }

  template <typename Callback>
  [[nodiscard]] bool matchAgainst(Quantity &remaining, Callback &&on_filled) {
    for (auto it = orders.begin(); remaining > 0 && it != orders.end();) {
      Order &order = *it++;

      Quantity delta = std::min(order.quantity, remaining);
      total_quantity -= delta;
      remaining -= delta;
      order.quantity -= delta;

      if (!order.quantity) {
        orders.erase(orders.iterator_to(order));
        on_filled(order);
      }
    }
    return empty();
  }

  [[nodiscard]] bool empty() const { return orders.empty(); }

  Quantity getQuantity() const { return total_quantity; }

private:
  OrderList orders;
  Quantity total_quantity{0};
};

template <typename T, typename Price, typename Quantity,
          typename Comparator = std::less<Price>>
class MarketSide {
public:
  using PriceLevel = ::PriceLevel<T>;
  using BookType = std::pmr::map<Price, PriceLevel, Comparator>;

  MarketSide(std::pmr::unsynchronized_pool_resource &pool) : book(&pool) {}

  void addOrder(T &order, Price price) {
    auto [it, added] = book.try_emplace(price);
    it->second.addOrder(order);
  }

  void removeOrder(T &order, Price price) {
    auto it = book.find(price);
    if (it != book.end() && it->second.removeOrder(order))
      book.erase(it);
  }

  template <typename Callback>
  void matchAgainst(Quantity &remaining, Price price, Callback &&on_filled) {
    const auto comp = book.key_comp();
    const auto level_end = book.end();

    auto level = book.begin();
    while (remaining > 0 && level != level_end && !comp(price, level->first)) {
      const bool cleared = level->second.matchAgainst(remaining, on_filled);
      level = cleared ? book.erase(level) : std::next(level);
    }
  }

  template <typename Level> std::optional<Level> getBest() const {
    auto it = book.begin();
    return it == book.end() ? std::nullopt
                            : Level{it->first, it->second.getQuantity()};
  }

  template <typename Level> std::vector<Level> getTop(uint16_t depth) const {
    std::vector<Level> result{};
    for (const auto &level : book) {
      if (result.size() >= depth)
        break;
      result.push_back({level.first, level.second.getQuantity()});
    }
    return result;
  }

  [[nodiscard]] bool empty() const { return book.empty(); }

private:
  BookType book;
};
