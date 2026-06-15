#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <stdexcept>
#include <vector>

using OrderID = uint64_t;

namespace ladder {

template <typename Quantity> struct Orders {
  struct OrderQuantity {
    OrderID order_id;
    Quantity quantity;
  };
  std::list<OrderQuantity> orders;
};

template <typename Price, typename Quantity, typename Comparator> class Ladder {
public:
  using BookType = std::map<Price, Orders<Quantity>, Comparator>;

  typename std::list<typename Orders<Quantity>::OrderQuantity>::iterator
  addOrder(OrderID order_id, Price price, Quantity quantity) {
    Orders<Quantity> orders;
    orders.orders.push_back({order_id, quantity});

    const auto [it, inserted] = book.try_emplace(price, orders);
    if (!inserted)
      it->second.orders.push_back({order_id, quantity});
    return --it->second.orders.end();
  }

  BookType &getBook() { return book; }
  const BookType &getBook() const { return book; }

  bool empty() const { return book.empty(); }

  // Need to provide access to the comparator for the matching logic
  auto key_comp() const { return book.key_comp(); }
  auto begin() { return book.begin(); }
  auto end() { return book.end(); }

  // Need to support erasing by iterator and returning next
  auto erase(typename BookType::iterator it) { return book.erase(it); }

private:
  BookType book;
};

template <typename Ladder, typename Level> Level getBest(const Ladder &ladder) {
  if (ladder.getBook().empty())
    throw std::runtime_error("Order Book Ladder: empty");
  return {ladder.getBook().begin()->first, ladder.getBook().begin()->second};
}

template <typename Ladder, typename Level>
std::vector<Level> getTop(const Ladder &ladder, uint16_t depth) {
  std::vector<Level> result{};
  result.reserve(depth);

  for (const auto &current : ladder.getBook()) {
    typename Level::Quantity acc{0};
    for (const auto &x : current.second.orders)
      acc += x.quantity;

    result.push_back({current.first, acc});
    if (result.size() >= depth)
      break;
  }
  return result;
}

} // namespace ladder
