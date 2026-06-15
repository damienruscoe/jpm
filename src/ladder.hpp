#pragma once

#include <cstdint>
#include <map>
#include <memory_resource>
#include <stdexcept>

using OrderID = uint64_t;

namespace ladder {

template <typename Quantity> struct Orders {
  struct OrderQuantity {
    OrderID order_id;
    Quantity quantity;
  };
  // std::list<{Quantity, OrderID}> orders;
  // std::list<Quantity> orders;
  std::list<OrderQuantity> orders;
};

template <typename Price, typename Quantity>
using BidBook = std::map<Price, Orders<Quantity>, std::greater<Price>>;

template <typename Price, typename Quantity>
using AskBook = std::map<Price, Orders<Quantity>, std::less<Price>>;

template <typename Price, typename Quantity>
using PmrBidBook = std::pmr::map<Price, Orders<Quantity>, std::greater<Price>>;

template <typename Price, typename Quantity>
using PmrAskBook = std::pmr::map<Price, Orders<Quantity>, std::less<Price>>;

template <typename Ladder, typename Level>
typename std::list<
    typename Orders<typename Level::Quantity>::OrderQuantity>::iterator
addOrder(OrderID order_id, Ladder &ladder, const Level &level) {
  Orders<typename Level::Quantity> orders;
  orders.orders.push_back({order_id, level.quantity});

  const auto [it, inserted] = ladder.try_emplace(level.price, orders);
  if (!inserted)
    it->second.orders.push_back({order_id, level.quantity});
  return --it->second.orders.end();
}

template <typename Level, typename Ladder> Level getBest(const Ladder &ladder) {
  if (ladder.empty())
    throw std::runtime_error("Order Book Ladder: empty");
  return {ladder.begin()->first, ladder.begin()->second};
}

template <typename Level, typename Ladder>
std::vector<Level> getTop(const Ladder &ladder, uint16_t depth) {
  std::vector<Level> result{};
  result.reserve(depth);

  for (const auto &current : ladder) {
    typename Level::Quantity acc{0};
    for (const auto &x : current.second.orders)
      acc += x.quantity;

    result.push_back({current.first, acc});
    if (result.size() > depth)
      break;
  }
  return result;
}

} // namespace ladder
