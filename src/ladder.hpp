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

#include "order.hpp"

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

template <typename Order> struct LevelData {
private:
  using Quantity = typename Order::Quantity;

  using OrderList = boost::intrusive::list<
      Order,
      boost::intrusive::member_hook<Order, boost::intrusive::list_member_hook<>,
                                    &Order::ladder_hook>>;
  OrderList orders;

public:
  Quantity total_qty{0};

  using iterator = typename OrderList::iterator;
  using const_iterator = typename OrderList::const_iterator;

  void addOrder(Order &order, Quantity qty) {
    orders.push_back(order);
    total_qty += qty;
  }

  void removeOrder(Order &order) {
    total_qty -= order.quantity;
    orders.erase(orders.iterator_to(order));
  }

  template <typename Callback>
  bool matchAgainst(Quantity &remaining, Callback &&on_filled) {
    for (auto it = orders.begin(); remaining > 0 && it != orders.end();) {
      Order &order = *it++;

      Quantity delta = std::min(order.quantity, remaining);
      total_qty -= delta;
      remaining -= delta;
      order.quantity -= delta;

      if (!order.quantity) {
        orders.erase(orders.iterator_to(order));
        on_filled(order);
      }
    }
    return empty();
  }

  bool empty() const { return orders.empty(); }
};

template <typename Price, typename Quantity,
          typename Comparator = std::less<Price>>
class Ladder {
public:
  using Order = ::Order<Price, Quantity>;
  using LevelData = ::LevelData<Order>;
  using BookType = std::pmr::map<Price, LevelData, Comparator>;

  Ladder(std::pmr::unsynchronized_pool_resource &pool) : book(&pool) {}

  void addOrder(Order *order, Price price, Quantity qty) {
    auto [it, added] = book.try_emplace(price, LevelData{});
    it->second.addOrder(*order, qty);
  }

  void removeOrder(Order &order) {
    if (auto it = book.find(order.price); it != book.end()) {
      it->second.removeOrder(order);
      if (it->second.empty())
        book.erase(it);
    }
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
    if (empty())
      return std::nullopt;
    auto it = book.begin();
    return Level{it->first, it->second.total_qty};
  }

  template <typename Level> std::vector<Level> getTop(uint16_t depth) const {
    std::vector<Level> result{};
    for (const auto &level : book) {
      if (result.size() >= depth)
        break;
      result.push_back({level.first, level.second.total_qty});
    }
    return result;
  }

  bool empty() const { return book.empty(); }

private:
  BookType book;
};
