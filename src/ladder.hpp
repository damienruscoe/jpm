#pragma once

#include <boost/intrusive/list.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

enum class side_t { BID, ASK };
using OrderID = std::string;

template <typename PriceT, typename QuantityT> struct Order {
  using Price = PriceT;
  using Quantity = QuantityT;

  OrderID id;
  Price price;
  Quantity quantity;
  side_t side;
  boost::intrusive::list_member_hook<> intrusive_list_hook;

  Order(OrderID _id, Price _price, Quantity _quantity, side_t _side)
      : id(std::move(_id)), price(_price), quantity(_quantity), side(_side) {}
};

template <typename T>
concept LadderOrder = requires(T o) {
  typename T::Price;
  typename T::Quantity;
  { o.quantity } -> std::convertible_to<typename T::Quantity>;
  {o.intrusive_list_hook};
};

template <LadderOrder Order> struct PriceLevel {
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
    for (auto it = orders.begin(); remaining > 0 && it != orders.end();) {
      Order &order = *it++;

      Quantity delta = std::min(order.quantity, remaining);
      total_quantity -= delta;
      remaining -= delta;
      order.quantity -= delta;

      if (!order.quantity) {
        orders.erase(orders.iterator_to(order));
        on_filled(order);
      }
    }
    return orders.empty();
  }

  //[[nodiscard]] bool empty() const { return orders.empty(); }

  Quantity getQuantity() const { return total_quantity; }

private:
  OrderList orders;
  Quantity total_quantity{0};
};
