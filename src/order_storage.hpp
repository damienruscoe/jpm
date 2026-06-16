#pragma once

#include <boost/intrusive/list.hpp>
#include <memory_resource>

template <typename OrderType> class OrderStorage {
  std::pmr::unsynchronized_pool_resource pool;
  std::pmr::polymorphic_allocator<char> allocator;

public:
  OrderStorage() : allocator(&pool) {}

  template <typename... Args> OrderType *createOrder(Args &&...args) {
    void *mem = allocator.allocate(sizeof(OrderType));
    return new (mem) OrderType(std::forward<Args>(args)...);
  }

  void destroyOrder(OrderType *order) {
    order->~OrderType();
    allocator.deallocate(reinterpret_cast<char *>(order), sizeof(OrderType));
  }
};
