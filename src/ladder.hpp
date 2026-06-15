#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <variant>
#include <vector>

#include "stable_index_vector.hpp"

using OrderID = uint64_t;

namespace ladder {

template <typename Price, typename Quantity,
          typename Comparator = std::less<Price>>
class Ladder {
public:
  using OrderList = std::list<size_t>;
  using ListIterator = OrderList::iterator;

  struct LevelData {
    OrderList orders;
    Quantity total_qty;
  };

  using BookType = std::map<Price, LevelData, Comparator>;

  ListIterator addOrder(size_t id, Price price, Quantity qty) {
    auto [it, added] = book.try_emplace(price, LevelData{OrderList{id}, qty});
    if (!added) {
      it->second.orders.push_back(id);
      it->second.total_qty += qty;
    }

    return std::prev(it->second.orders.end());
  }

  void removeOrder(Price price, ListIterator it, Quantity qty) {
    auto book_it = book.find(price);
    if (book_it == book.end())
      return;

    book_it->second.orders.erase(it);
    book_it->second.total_qty -= qty;
    if (book_it->second.orders.empty())
      book.erase(book_it);
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

} // namespace ladder
