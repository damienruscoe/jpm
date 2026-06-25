#pragma once

#include <memory_resource>

template <typename T,
          typename Resource = std::pmr::unsynchronized_pool_resource>
class ObjectPool {

public:
  template <typename... Args> T *create(Args &&...args) {
    void *mem = pool.allocate(sizeof(T), alignof(T));
    return new (mem) T(std::forward<Args>(args)...);
  }

  void destroy(T *order) {
    order->~T();
    pool.deallocate(order, sizeof(T), alignof(T));
  }

#if 0
	void warm_memory(size_t total_bytes) {
		// Iterate over pages of memory and write a byte to each page.
		// This will ensure that page faults are avoided on first access preventing jitter
	}
#endif

private:
  Resource pool;
};
