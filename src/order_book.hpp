#pragma once

#include <list>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include "fixed_point.hpp"
#include "ladder.hpp"
#include "stable_index_vector.hpp"

enum class side_t { BID, ASK };

using OrderID = uint64_t;

template <typename Level> class OrderBook {
public:
  using Price = typename Level::Price;
  using Quantity = typename Level::Quantity;
  using Bids = Ladder<Price, Quantity, std::greater<Price>>;
  using Asks = Ladder<Price, Quantity, std::less<Price>>;

  struct LadderIt {
    bool is_bid;
    typename Bids::ListIterator it;
  };

  struct Order {
    OrderID id;
    Price price;
    Quantity quantity;
    side_t side;
    // The back-pointer to the ladder's list
    LadderIt ladder_it;
  };

  [[nodiscard]] bool update(OrderID order_id, side_t side, Level level);

  std::optional<Level> getBest(side_t side) const;
  std::optional<Level> getBestBid() const;
  std::optional<Level> getBestAsk() const;

  std::vector<Level> getTop(side_t side, uint16_t depth = 10) const;
  std::vector<Level> getTopBid(uint16_t depth = 10) const;
  std::vector<Level> getTopAsk(uint16_t depth = 10) const;

private:
  template <typename LadderType>
  void matchAgainst(Quantity &remaining, Price price,
                    LadderType &opposingLadder, const auto &comp);

  template <typename LadderType>
  void addToLadder(OrderID order_id, Price price, Quantity qty, side_t side,
                   LadderType &ladder);

  StableVector<Order> order_pool;
  std::unordered_map<OrderID, size_t> id_to_siv_id;

  Bids bids;
  Asks asks;
};

template <typename Level>
template <typename LadderType>
void OrderBook<Level>::matchAgainst(Quantity &remaining, Price price,
                                    LadderType &opposingLadder,
                                    const auto &comp) {
  auto &book = opposingLadder.getBook();
  while (remaining > 0 && !book.empty() && !comp(book.begin()->first, price)) {
    const Price &current_price = book.begin()->first;
    auto &level_data = book.begin()->second;

    for (auto it = level_data.orders.begin();
         remaining > 0 && it != level_data.orders.end();) {
      auto *order = order_pool.get(*it);

      if (order->quantity <= remaining) {
        remaining -= order->quantity;
        id_to_siv_id.erase(order->id);

        order_pool.erase(*it);
        opposingLadder.removeOrder(current_price, it++, order->quantity);
      } else {
        order->quantity -= remaining;
        level_data.total_qty -= remaining;
        remaining = 0;
      }
    }
  }
}

template <typename Level>
template <typename LadderType>
void OrderBook<Level>::addToLadder(OrderID order_id, Price price, Quantity qty,
                                   side_t side, LadderType &ladder) {
  size_t id = order_pool.emplace_back(
      Order{order_id,
            price,
            qty,
            side,
            {side == side_t::BID, {typename Bids::ListIterator{}}}});
  id_to_siv_id[order_id] = id;
  order_pool.get(id)->ladder_it.it = ladder.addOrder(id, price, qty);
}

template <typename Level>
bool OrderBook<Level>::update(OrderID order_id, side_t side, Level level) {
  if (id_to_siv_id.find(order_id) != id_to_siv_id.end())
    return false;

  Quantity remaining = level.quantity;

  if (side == side_t::BID) {
    matchAgainst(remaining, level.price, asks, bids.key_comp());
    if (remaining > 0)
      addToLadder(order_id, level.price, remaining, side, bids);
  } else {
    matchAgainst(remaining, level.price, bids, asks.key_comp());
    if (remaining > 0)
      addToLadder(order_id, level.price, remaining, side, asks);
  }

  return true;
}

template <typename Level>
std::optional<Level> OrderBook<Level>::getBest(side_t side) const {
  return side == side_t::ASK ? getBestAsk() : getBestBid();
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTop(side_t side, uint16_t depth) const {
  return side == side_t::ASK ? getTopAsk(depth) : getTopBid(depth);
}

template <typename Level>
std::optional<Level> OrderBook<Level>::getBestAsk() const {
  return asks.template getBest<Level>();
}

template <typename Level>
std::optional<Level> OrderBook<Level>::getBestBid() const {
  return bids.template getBest<Level>();
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopAsk(uint16_t depth) const {
  return asks.template getTop<Level>(depth);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopBid(uint16_t depth) const {
  return bids.template getTop<Level>(depth);
}
