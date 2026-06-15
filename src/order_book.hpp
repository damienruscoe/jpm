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

namespace ladder {
template <typename Price, typename Quantity, typename Comparator> class Ladder;
}

template <typename Level> class OrderBook {
public:
  using Price = typename Level::Price;
  using Quantity = typename Level::Quantity;
  using Bids = ladder::Ladder<Price, Quantity, std::greater<Price>>;
  using Asks = ladder::Ladder<Price, Quantity, std::less<Price>>;

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
  siv::Vector<Order> order_pool;
  std::unordered_map<OrderID, siv::ID> id_to_siv_id;

  Bids bids;
  Asks asks;
};

template <typename Level>
bool OrderBook<Level>::update(OrderID order_id, side_t side, Level level) {
  if (id_to_siv_id.find(order_id) != id_to_siv_id.end())
    return false;

  Quantity remaining = level.quantity;

  auto fetchOrder = [&](siv::ID id) { return order_pool.get(id); };

  if (side == side_t::BID) {
    // Matching Bids (new) against Asks (opposing)
    const auto price_exceeded = bids.key_comp();
    while (remaining > 0 && !asks.empty() &&
           !price_exceeded(asks.getBook().begin()->first, level.price)) {
      auto price_it = asks.getBook().begin();
      auto &orders = price_it->second;
      for (auto it = orders.begin(); remaining > 0 && it != orders.end();) {
        auto *order = fetchOrder(*it);

        if (order->quantity <= remaining) {
          remaining -= order->quantity;
          id_to_siv_id.erase(order->id);
          order_pool.erase(*it);
          it = orders.erase(it);
        } else {
          order->quantity -= remaining;
          remaining = 0;
        }
      }
      if (orders.empty())
        asks.removeBest();
    }

    if (remaining > 0) {
      siv::ID id = order_pool.emplace_back(
          Order{order_id,
                level.price,
                remaining,
                side,
                {true, {typename Bids::ListIterator{}}}});
      id_to_siv_id[order_id] = id;

      order_pool.get(id)->ladder_it.it = bids.addOrder(id, level.price);
    }
  } else {
    // Matching Asks (new) against Bids (opposing)
    const auto price_exceeded = asks.key_comp();
    while (remaining > 0 && !bids.empty() &&
           !price_exceeded(bids.getBook().begin()->first, level.price)) {
      auto price_it = bids.getBook().begin();
      auto &orders = price_it->second;
      for (auto it = orders.begin(); remaining > 0 && it != orders.end();) {
        auto *order = fetchOrder(*it);

        if (order->quantity <= remaining) {
          remaining -= order->quantity;
          id_to_siv_id.erase(order->id);
          order_pool.erase(*it);
          it = orders.erase(it);
        } else {
          order->quantity -= remaining;
          remaining = 0;
        }
      }
      if (orders.empty())
        bids.removeBest();
    }

    if (remaining > 0) {
      siv::ID id = order_pool.emplace_back(
          Order{order_id,
                level.price,
                remaining,
                side,
                {false, {typename Asks::ListIterator{}}}});
      id_to_siv_id[order_id] = id;

      order_pool.get(id)->ladder_it.it = asks.addOrder(id, level.price);
    }
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
  auto fetchOrder = [&](siv::ID id) { return order_pool.get(id); };
  return asks.template getBest<Level>(fetchOrder);
}

template <typename Level>
std::optional<Level> OrderBook<Level>::getBestBid() const {
  auto fetchOrder = [&](siv::ID id) { return order_pool.get(id); };
  return bids.template getBest<Level>(fetchOrder);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopAsk(uint16_t depth) const {
  auto fetchOrder = [&](siv::ID id) { return order_pool.get(id); };
  return asks.template getTop<Level>(depth, fetchOrder);
}

template <typename Level>
std::vector<Level> OrderBook<Level>::getTopBid(uint16_t depth) const {
  auto fetchOrder = [&](siv::ID id) { return order_pool.get(id); };
  return bids.template getTop<Level>(depth, fetchOrder);
}
