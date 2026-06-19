#pragma once

#include <boost/intrusive/list.hpp>
#include <functional>

enum class side_t { BID, ASK };

template <typename OrderID, typename PriceT, typename QuantityT> struct Order {
  using Price = PriceT;
  using Quantity = QuantityT;

  OrderID id;
  Price price;
  Quantity quantity;
  boost::intrusive::list_member_hook<> intrusive_list_hook;

  template <typename _OrderID>
  Order(_OrderID &&_id, Price _price, Quantity _quantity)
      : id(std::forward<_OrderID>(_id)), price(std::move(_price)),
        quantity(std::move(_quantity)) {}
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
      Quantity &quantity = order.quantity;

      Quantity delta = std::min(quantity, remaining);
      total_quantity -= delta;
      quantity -= delta;
      remaining -= delta;

      if (!quantity) {
        orders.erase(orders.iterator_to(order));
        std::invoke(on_filled, order);
      }
    }
    return orders.empty();
  }

  Quantity getQuantity() const { return total_quantity; }

private:
  OrderList orders;
  Quantity total_quantity{0};
};
