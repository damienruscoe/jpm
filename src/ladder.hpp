#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>
#include <boost/intrusive/list.hpp>

#include "order.hpp"

template <typename Price, typename Quantity,
          typename Comparator = std::less<Price>>
class Ladder {
public:
  using OrderType = ::Order<Price, Quantity>;
  using OrderList = boost::intrusive::list<OrderType, 
    boost::intrusive::member_hook<OrderType, 
        boost::intrusive::list_member_hook<>, 
        &OrderType::ladder_hook>>;

  struct LevelData {
    OrderList orders;
    Quantity total_qty;
  };

  using BookType = std::map<Price, LevelData, Comparator>;

  auto addOrder(OrderType* order, Price price, Quantity qty) {
    auto [it, added] = book.try_emplace(price, LevelData{OrderList{}, qty});
    if (!added) {
      it->second.total_qty += qty;
    }
    it->second.orders.push_back(*order);

    return it->second.orders.iterator_to(*order);
  }

  void removeOrder(Price price, OrderType* order, Quantity qty) {
    if (auto it = book.find(price); it != book.end()) {
      it->second.orders.erase(it->second.orders.iterator_to(*order));
      it->second.total_qty -= qty;
      if (it->second.orders.empty())
        book.erase(it);
    }
  }

  void removeBest() {
    if (!book.empty()) {
      book.erase(book.begin());
    }
  }

  BookType &getBook() { return book; }
  const BookType &getBook() const { return book; }
  auto key_comp() const { return book.key_comp(); }

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
