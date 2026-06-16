#pragma once

#include <boost/intrusive/list.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

#include "order.hpp"
#include "order_storage.hpp"

template <typename Price, typename Quantity,
          typename Comparator = std::less<Price>>
class Ladder {
public:
  using OrderType = ::Order<Price, Quantity>;

  struct LevelData {
  private:
    using OrderList = boost::intrusive::list<
        OrderType, boost::intrusive::member_hook<
                       OrderType, boost::intrusive::list_member_hook<>,
                       &OrderType::ladder_hook>>;
    OrderList orders;

  public:
    Quantity total_qty{0};

    void addOrder(OrderType &order, Quantity qty) {
      orders.push_back(order);
      total_qty += qty;
    }

    void removeOrder(OrderType &order, Quantity qty) {
      orders.erase(orders.iterator_to(order));
      total_qty -= qty;
    }

    bool consumeOrder(OrderType &order, Quantity &remaining) {
      if (order.quantity <= remaining) {
        remaining -= order.quantity;
        total_qty -= order.quantity;
        orders.erase(orders.iterator_to(order));
        return true; // Order fully removed
      } else {
        order.quantity -= remaining;
        total_qty -= remaining;
        remaining = 0;
        return false; // Order partially filled
      }
    }

    bool empty() const { return orders.empty(); }
    auto iterator_to(OrderType &order) { return orders.iterator_to(order); }
    auto begin() { return orders.begin(); }
    auto end() { return orders.end(); }
  };

  using BookType = std::map<Price, LevelData, Comparator>;

  auto addOrder(OrderType *order, Price price, Quantity qty) {
    auto [it, added] = book.try_emplace(price, LevelData{});
    it->second.addOrder(*order, qty);
    return it->second.iterator_to(*order);
  }

  void removeOrder(Price price, OrderType *order, Quantity qty) {
    if (auto it = book.find(price); it != book.end()) {
      it->second.removeOrder(*order, qty);
      if (it->second.empty())
        book.erase(it);
    }
  }

  template <typename MapType>
  void matchAgainst(Quantity &remaining, Price price, MapType &id_to_order_ptr,
                    OrderStorage<OrderType> &storage) {
    const auto comp = book.key_comp();
    const auto level_end = book.end();

    auto level = book.begin();
    while (remaining && level != level_end && !comp(price, level->first)) {
      auto &level_data = level->second;
      const auto orders_end = level_data.end();

      for (auto it = level_data.begin(); remaining && it != orders_end;) {
        OrderType *order = &(*it);
        ++it;

        if (level_data.consumeOrder(*order, remaining)) {
          id_to_order_ptr.erase(order->id);
          storage.destroyOrder(order);
        }
      }

      if (level_data.empty()) {
        level = book.erase(level);
      } else {
        ++level;
      }
    }
  }

  void removeBest() {
    if (!book.empty()) {
      book.erase(book.begin());
    }
  }

  BookType &getBook() { return book; }
  const BookType &getBook() const { return book; }

  template <typename Level> std::optional<Level> getBest() const {
    if (book.empty())
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
