#pragma once

#include "order_book.hpp"
#include "order_book_side.hpp"

#include <optional>
#include <vector>

#include "signals/signals_base.hpp"

template <typename Traits_T> struct OrderBookTraitsL2 {
  using OrderID = typename Traits_T::Price;
  using Price = typename Traits_T::Price;
  using Quantity = typename Traits_T::Quantity;

  using Traits = OrderBookTraits<Price, Price, Quantity>;
};

template <typename Traits,
          typename SignalAggregator = signals::EmptySignals<Traits>>
class OrderBookL2Adapter {
public:
  using UnderlyingBook = OrderBook<Traits, SignalAggregator>;

  using StoredOrderID = typename UnderlyingBook::StoredOrderID;
  using Price = typename UnderlyingBook::Price;
  using Quantity = typename UnderlyingBook::Quantity;

  using Order = typename UnderlyingBook::Order;
  using Orders = typename UnderlyingBook::Orders;
  using Aggregator = typename UnderlyingBook::Aggregator;
  using L2PriceLevel = typename UnderlyingBook::L2PriceLevel;

  [[nodiscard]] bool setPriceLevel(side_t side, const Price &price,
                                   const Quantity &qty) {
    if (!m_book.amend(price, side, price, qty))
      return m_book.newOrder(price, side, price, qty);
    return false;
  }

  bool isCrossedOrderBook() const { return m_book.isCrossedOrderBook(); }

  std::optional<L2PriceLevel> getBestAsk() const { return m_book.getBestAsk(); }

  std::optional<L2PriceLevel> getBestBid() const { return m_book.getBestBid(); }

  std::vector<L2PriceLevel> getTopAsks(uint16_t depth = 10) const {
    return m_book.getTopAsks(depth);
  }

  std::vector<L2PriceLevel> getTopBids(uint16_t depth = 10) const {
    return m_book.getTopBids(depth);
  }

  const Orders &getOrders() const { return m_book.getOrders(); }

  Aggregator &getSignals() { return m_book.getSignals(); }
  const Aggregator &getSignals() const { return m_book.getSignals(); }

private:
  UnderlyingBook m_book;
};
