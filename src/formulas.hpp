#pragma once

template <typename L2> struct Formulas {
  using Price = decltype(L2::price);

  static Price spread(const L2 &best_bid, const L2 &best_ask) {
    return best_ask.price - best_bid.price;
  }

  static Price midPrice(const L2 &best_bid, const L2 &best_ask) {
    return (best_ask.price + best_bid.price) / Price{2};
  }

  static Price weightedMidPrice(const L2 &best_bid, const L2 &best_ask) {
    return (best_ask.price * best_bid.quantity +
            best_bid.price * best_ask.quantity) /
           (best_ask.quantity + best_bid.quantity);
  }
};
