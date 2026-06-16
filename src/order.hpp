#pragma once

#include <boost/intrusive/list.hpp>
#include <string>

enum class side_t { BID, ASK };
using OrderID = std::string;

template <typename Price, typename Quantity> struct Order {
  OrderID id;
  Price price;
  Quantity quantity;
  side_t side;
  boost::intrusive::list_member_hook<> ladder_hook;

  Order(OrderID _id, Price _price, Quantity _quantity, side_t _side)
      : id(std::move(_id)), price(_price), quantity(_quantity), side(_side) {}
};
