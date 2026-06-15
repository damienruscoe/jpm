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
  using OrderList = std::list<siv::ID>;
  using ListIterator = OrderList::iterator;

  using BookType = std::map<Price, OrderList, Comparator>;

  ListIterator addOrder(siv::ID id, Price price) {
    auto [it, added] = book.try_emplace(price, OrderList{id});
    if (!added)
      it->second.push_back(id);

    return std::prev(it->second.end());
  }

  void removeOrder(Price price, ListIterator it) {
    auto book_it = book.find(price);
    if (book_it == book.end())
      return;

    book_it->second.erase(it);
    if (book_it->second.empty())
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

  template <typename Level>
  std::optional<Level> getBest(auto fetchOrder) const {
    if (book.empty())
      return std::nullopt;
    auto it = book.begin();
    Quantity acc{0};
    for (const auto &id : it->second) {
      if (auto *order = fetchOrder(id)) {
        acc += order->quantity;
      }
    }
    return Level{it->first, acc};
  }

  template <typename Level>
  std::vector<Level> getTop(uint16_t depth, auto fetchOrder) const {
    std::vector<Level> result{};
    for (const auto &level : book) {
      if (result.size() >= depth)
        break;
      Quantity acc{0};
      for (const auto &id : level.second) {
        if (auto *order = fetchOrder(id)) {
          acc += order->quantity;
        }
      }
      result.push_back({level.first, acc});
    }
    return result;
  }

  bool empty() const { return book.empty(); }

private:
  BookType book;
};

} // namespace ladder
