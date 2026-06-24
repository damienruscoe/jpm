#pragma once

#include "order_book_side.hpp"
#include "order_id.hpp"

#include <memory_resource>
#include <optional>
#include <vector>

#include "object_resource.hpp"

template <typename T, typename ReturnType = decltype(T::id)> struct GetIdField {
  static ReturnType get(const T *t) { return t->id; };
};

template <typename OrderID_t, typename Price_t, typename Quantity_t>
class OrderBook {
public:
  using Price = Price_t;
  using Quantity = Quantity_t;
  using StoredOrderID = OrderID_t;

  struct L2PriceLevel {
    Price price;
    Quantity quantity;
  };

  template <typename OrderID>
  [[nodiscard]] bool newOrder(OrderID &&order_id, side_t side,
                              const Price &price, const Quantity &qty);
  template <typename OrderID>
  [[nodiscard]] bool cancel(OrderID &&order_id, side_t side);
  template <typename OrderID>
  [[nodiscard]] bool amend(OrderID &&order_id, side_t side, const Price &price,
                           const Quantity &qty);

  std::optional<L2PriceLevel> getBestAsk() const {
    return asks.template getBest<L2PriceLevel>();
  }

  std::optional<L2PriceLevel> getBestBid() const {
    return bids.template getBest<L2PriceLevel>();
  }

  std::vector<L2PriceLevel> getTopAsks(uint16_t depth = 10) const {
    return asks.template getTop<L2PriceLevel>(depth);
  }

  std::vector<L2PriceLevel> getTopBids(uint16_t depth = 10) const {
    return bids.template getTop<L2PriceLevel>(depth);
  }

  std::optional<Price> getSpread() const {
    auto best_bid = getBestBid();
    auto best_ask = getBestAsk();
    if (best_bid && best_ask)
      return best_ask->price - best_bid->price;
    return std::nullopt;
  }

private:
  using Order = ::Order<StoredOrderID, Price, Quantity>;
  using BidsBook = OrderBookSide<Order, std::greater<Price>>;
  using AsksBook = OrderBookSide<Order, std::less<Price>>;
  using Orders = ObjectResource<Order, GetIdField<Order, StoredOrderID>>;

  struct CrossingTradeDispatcher {

    template <typename OrderID>
    static void emplaceOrder(OrderBook &order_book, auto &aggressor,
                             auto &resting, OrderID &&order_id,
                             const Price &price, const Quantity &qty,
                             side_t side) {
      auto on_filled =
          std::bind_front(&OrderBook::on_filled, &order_book, side);
      if (const auto remaining =
              aggressor.matchPrice(price, qty, std::move(on_filled))) {
        auto *order = order_book.orders.create(std::forward<OrderID>(order_id),
                                               price, remaining, side);
        resting.insertOrder(*order);
      }
    }

    static bool amend(OrderBook &order_book, auto &aggressor, auto &resting,
                      Order *order, const Price &price, const Quantity &qty) {
      auto on_filled =
          std::bind_front(&OrderBook::on_filled, &order_book, order->side);
      const auto remaining =
          aggressor.matchPrice(price, qty, std::move(on_filled));
      resting.amendOrder(*order, price, remaining);
      return true;
    }
  };

  void on_filled(side_t side, const OrderID_t &filled_order_id,
                 const Price &price, const Quantity &qty, bool partial) {
    constexpr std::string_view TRADE = "[\033[94mTRADE\033[0m] ";

    const int ticker = 101;
    const char aggressor_side = side == side_t::ASK ? 'S' : 'B';

    std::cout << TRADE << "Trade: " << ticker << " " << filled_order_id << " "
              << qty << " " << price << ", AggrSide=" << aggressor_side
              << (partial ? " (Partial)" : "") << std::endl;

    if (!partial)
      orders.erase(filled_order_id);
  };

  Orders orders;

  std::pmr::unsynchronized_pool_resource pool;
  BidsBook bids{pool};
  AsksBook asks{pool};
};

template <typename OrderID, typename Price, typename Quantity>
template <typename OrderID_Param>
bool OrderBook<OrderID, Price, Quantity>::newOrder(OrderID_Param &&order_id,
                                                   side_t side,
                                                   const Price &price,
                                                   const Quantity &qty) {

  if (orders.contains(order_id))
    return false;

  side == side_t::BID
      ? CrossingTradeDispatcher::emplaceOrder(
            *this, asks, bids, std::forward<OrderID_Param>(order_id), price,
            qty, side)
      : CrossingTradeDispatcher::emplaceOrder(
            *this, bids, asks, std::forward<OrderID_Param>(order_id), price,
            qty, side);

  return true;
}

template <typename OrderID, typename Price, typename Quantity>
template <typename OrderID_Param>
bool OrderBook<OrderID, Price, Quantity>::cancel(OrderID_Param &&order_id,
                                                 side_t side) {
  if (auto *order = orders.find(std::forward<OrderID_Param>(order_id))) {
    side == side_t::BID ? bids.removeOrder(*order) : asks.removeOrder(*order);
    orders.erase(order->id);
    return true;
  }
  return false;
}

template <typename OrderID, typename Price, typename Quantity>
template <typename OrderID_Param>
bool OrderBook<OrderID, Price, Quantity>::amend(OrderID_Param &&order_id,
                                                side_t side, const Price &price,
                                                const Quantity &qty) {
  if (auto *order = orders.find(std::forward<OrderID_Param>(order_id))) {
    if (qty < order->quantity && price == order->price) {
      // Reducing quantity only. Maintain PriceTime priority.
      // This action cannot create any trades against resting.
      // Updating the order quanity is the only action that needs to be taken.
      order->quantity = qty;
      return true;
    }

    return order->side == side &&
           (side == side_t::BID ? CrossingTradeDispatcher::amend(
                                      *this, asks, bids, order, price, qty)
                                : CrossingTradeDispatcher::amend(
                                      *this, bids, asks, order, price, qty));
  }
  return false;
}
