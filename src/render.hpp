#pragma once

#include <iostream>
#include <sstream>
#include <utility>

#include "order_book.hpp"

std::ostream &nl(std::ostream &os);

template <typename Level> class TopOfBookRender {
public:
  void render(const OrderBook<Level> &book) {
    const auto bid = book.getBestBid();
    const auto ask = book.getBestAsk();
    if (bid && ask) {
      std::cout << *bid << " - " << *ask << nl;
    }
  }

private:
  std::pair<std::optional<Level>, std::optional<Level>> prev_bests =
      std::make_pair(std::nullopt, std::nullopt);
};

template <typename Level> void render_book(const OrderBook<Level> &book) {
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
