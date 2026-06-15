#pragma once

#include <iostream>
#include <sstream>
#include <utility>

#include "order_book.hpp"

std::ostream &nl(std::ostream &os);

template <typename Level> class TopOfBookRender {
public:
  void render(const OrderBook<Level> &book) {
    const auto new_bests = std::make_pair(book.getBestBid(), book.getBestAsk());
    if (new_bests != prev_bests) {
      prev_bests = new_bests;
      const auto &[bid, ask] = new_bests;
      std::cout << bid << " - " << ask << nl;
    }
  }

private:
  std::pair<Level, Level> prev_bests = std::make_pair(Level{}, Level{});
};

template <typename Level> void render_book(const OrderBook<Level> &book) {
  const auto top_asks = book.getTopAsk();
  const auto top_bids = book.getTopBid();

  std::stringstream ss;

  // ss << "\033[H\n"; // Move cursor to top left of the screen

  for (auto it = top_asks.crbegin(); it != top_asks.crend(); ++it)
    ss << *it << nl;

  ss << "---------------------------" << nl;
  // ss << "\033[K"; // Clear to the end of line
  if (top_asks.empty() || top_bids.empty()) {
    ss << "Spread: " << nl;
  } else {
    const auto spread = top_asks[0].price - top_bids[0].price;
    ss << "Spread: " << spread << nl;
  }
  ss << "---------------------------" << nl;

  for (auto it = top_bids.cbegin(); it != top_bids.cend(); ++it)
    ss << *it << nl;

  ss << nl;
  std::cout << ss.str() << std::flush;
}
