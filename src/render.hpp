#pragma once

#include <iostream>
#include <sstream>
#include <utility>

#include "order_book.hpp"

std::ostream &nl(std::ostream &os);

template <typename OrderBook>
concept RenderableOrderBook = requires(const OrderBook &book) {
  book.getBestBid();
  book.getBestAsk();
};

template <RenderableOrderBook OrderBook> class TopOfBookRenderer {
public:
  TopOfBookRenderer(const OrderBook &book) : m_book(book) {}

  void render() {
    const auto bid = m_book.getBestBid();
    const auto ask = m_book.getBestAsk();
    auto new_bests = std::make_pair(bid, ask);

    if (bid && ask && new_bests != prev_bests) {
      std::cout << *bid << " - " << *ask << nl;
    }
  }

private:
  const OrderBook &m_book;

  using BidLevel =
      typename decltype(std::declval<OrderBook>().getBestBid())::value_type;
  using AskLevel =
      typename decltype(std::declval<OrderBook>().getBestAsk())::value_type;

  std::pair<std::optional<BidLevel>, std::optional<AskLevel>> prev_bests =
      std::make_pair(std::nullopt, std::nullopt);
};

template <RenderableOrderBook OrderBook>
void render_book(const OrderBook &book) {
  const auto top_asks = book.getTopAsks();
  const auto top_bids = book.getTopBids();

  std::stringstream ss;

  for (auto it = top_asks.crbegin(); it != top_asks.crend(); ++it)
    ss << it->price << ' ' << it->quantity << nl;

  ss << "---------------------------" << nl;
#if 0
  if (auto spread = book.getSpread())
    ss << "Spread: " << *spread << nl
       << "---------------------------" << nl;
#endif

  for (auto it = top_bids.cbegin(); it != top_bids.cend(); ++it)
    ss << it->price << ' ' << it->quantity << nl;

  ss << nl;
  std::cout << ss.str() << std::flush;
}
