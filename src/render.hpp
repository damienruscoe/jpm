#pragma once

#include <iostream>
#include <sstream>
#include <utility>

#include "order_book.hpp"

std::ostream &nl(std::ostream &os);

template <typename OrderBook> class TopOfBookRender {
public:
  void render(const OrderBook &book) {
    const auto bid = book.getBestBid();
    const auto ask = book.getBestAsk();
    auto new_bests = std::make_pair(bid, ask);

    if (bid && ask && new_bests != prev_bests) {
      std::cout << *bid << " - " << *ask << nl;
    }
  }

private:
  using BidLevel =
      typename decltype(std::declval<OrderBook>().getBestBid())::value_type;
  using AskLevel =
      typename decltype(std::declval<OrderBook>().getBestAsk())::value_type;

  std::pair<std::optional<BidLevel>, std::optional<AskLevel>> prev_bests =
      std::make_pair(std::nullopt, std::nullopt);
};

template <typename OrderBook> void render_book(const OrderBook &book) {
  const auto top_asks = book.getTopAsk();
  const auto top_bids = book.getTopBid();

  std::stringstream ss;

  for (auto it = top_asks.crbegin(); it != top_asks.crend(); ++it)
    ss << *it << nl;

  ss << "---------------------------" << nl;

  if (top_asks.empty() || top_bids.empty()) {
    ss << "Spread: " << nl;
  } else {
    // Revert to original spread rendering: (ask - bid)
    auto spread = top_asks[0].price - top_bids[0].price;
    ss << "Spread: " << spread << nl;
  }

  ss << "---------------------------" << nl;

  for (auto it = top_bids.cbegin(); it != top_bids.cend(); ++it)
    ss << *it << nl;

  ss << nl;
  std::cout << ss.str() << std::flush;
}
