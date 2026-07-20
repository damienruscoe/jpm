#pragma once

#include <boost/intrusive/list.hpp>
#include <functional>

#include "core/enums.hpp"

// template <typename OrderID, typename PriceT, typename QuantityT> struct Order
// {
template <typename TraitsT> struct Order {
  using Traits = TraitsT;
  using OrderID = typename Traits::OrderID;
  using Price = typename Traits::Price;
  using Quantity = typename Traits::Quantity;

  OrderID id;
  Price price;
  Quantity quantity;
  side_t side;
  boost::intrusive::list_member_hook<> intrusive_list_hook;

  template <typename _OrderID>
  Order(_OrderID &&_id, Price _price, Quantity _quantity, side_t _side)
      : id(std::forward<_OrderID>(_id)), price(std::move(_price)),
        quantity(std::move(_quantity)), side(_side) {}
};

template <typename Order> struct PriceLevel {
private:
  using Quantity = typename Order::Quantity;

  using OrderList = boost::intrusive::list<
      Order,
      boost::intrusive::member_hook<Order, boost::intrusive::list_member_hook<>,
                                    &Order::intrusive_list_hook>>;

public:
  void addOrder(Order &order) {
    orders.push_back(order);
    total_quantity += order.quantity;
  }

  [[nodiscard]] bool removeOrder(const Order &order) {
    total_quantity -= order.quantity;
    orders.erase(orders.iterator_to(order));
    return orders.empty();
  }

  template <typename Callback>
  [[nodiscard]] bool matchAgainst(Quantity &remaining, Callback &&on_filled) {
    const auto end = orders.end();
    for (auto it = orders.begin(); remaining > 0 && it != end;) {
      // The OrderList is being modified while we are iterating, so bump
      // the iterator now before it becomes invalid.
      Order &order = *it++;

      Quantity delta = std::min(order.quantity, remaining);
      total_quantity -= delta;
      order.quantity -= delta;
      remaining -= delta;

      if (!order.quantity)
        // Remove this order from the PriceLevel linked list
        // The order is not destructed.
        orders.erase(orders.iterator_to(order));

      // This may destroy the order. Do not access order after invoking.
      std::invoke(on_filled, order.id, order.price, delta,
                  order.quantity ? FillStatus::Partial : FillStatus::Full);
    }
    return orders.empty();
  }

  Quantity getQuantity() const { return total_quantity; }
  uint32_t getOrderCount() const { return orders.size(); }

private:
  OrderList orders;
  Quantity total_quantity{0};
};
