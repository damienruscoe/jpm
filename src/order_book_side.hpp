#pragma once

#include <map>
#include <memory_resource>
#include <optional>

#include "price_level.hpp"

template <typename Order, typename Comparitor> class OrderBookSide {
public:
  using Price = typename Order::Price;
  using Quantity = typename Order::Quantity;

  OrderBookSide(std::pmr::unsynchronized_pool_resource &pool) : book(&pool) {}

  template <typename OrderCallback>
  Quantity matchPrice(const Price &price, Quantity qty,
                      OrderCallback &&on_filled) {
    const auto comp = typename Book::key_compare{};
    const auto level_end = book.end();

    auto level = book.begin();
    while (qty > 0 && level != level_end && !comp(price, level->first)) {
      const bool cleared = level->second.matchAgainst(
          qty, std::forward<OrderCallback>(on_filled));
      level = cleared ? book.erase(level) : std::next(level);
    }

    return qty;
  }

  void insertOrder(Order &order) { book[order.price].addOrder(order); }

  void removeOrder(Order &order) {
    if (const auto it = book.find(order.price);
        it != book.end() && it->second.removeOrder(order))
      book.erase(it);
  }

  template <typename Level> std::optional<Level> getBest() const {
    auto it = book.begin();
    return it == book.end()
               ? std::nullopt
               : std::make_optional(Level{it->first, it->second.getQuantity(),
                                          it->second.getQuantity()});
  }

  template <typename Level> std::vector<Level> getTop(uint16_t depth) const {
    using Quantity = decltype(std::declval<Level>().quantity);
    Quantity total{};

    std::vector<Level> result{};
    for (const auto &[price, price_orders] : book) {
      if (result.size() >= depth)
        break;
      auto current_quantity = price_orders.getQuantity();
      total += current_quantity;
      result.push_back({price, current_quantity, total});
    }
    return result;
  }

private:
  using Book = std::pmr::map<Price, PriceLevel<Order>, Comparitor>;
  Book book;
};
