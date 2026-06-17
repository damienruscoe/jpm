#pragma once

#include <boost/intrusive/list.hpp>
#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <vector>

/*
  1. Define a Tracking Resource

    1 class TrackingResource : public std::pmr::memory_resource {
    2     std::pmr::memory_resource* upstream_;
    3     size_t total_allocated_ = 0;
    4 public:
    5     TrackingResource(std::pmr::memory_resource* upstream) :
  upstream_(upstream) {}
    6
    7     void* do_allocate(size_t bytes, size_t alignment) override {
    8         total_allocated_ += bytes; // Track it!
    9         return upstream_->allocate(bytes, alignment);
   10     }
   11     void do_deallocate(void* p, size_t bytes, size_t alignment) override {
   12         upstream_->deallocate(p, bytes, alignment);
   13     }
   14     bool do_is_equal(const memory_resource& other) const noexcept override
  { 15         return this == &other; 16     } 17     size_t
  get_total_allocated() const { return total_allocated_; } 18 };

  2. Integrate with ObjectPool

   1 class ObjectPool {
   2     TrackingResource tracker{std::pmr::get_default_resource()};
   3     std::pmr::unsynchronized_pool_resource pool{&tracker}; // Pool uses
  tracker 4 public: 5     // ... 6     size_t get_reserved_memory() const {
  return tracker.get_total_allocated(); } 7 };
 */

template <typename T,
          typename Resource = std::pmr::unsynchronized_pool_resource>
class ObjectPool {

public:
  // ObjectPool() : allocator(&pool) {
  /*
std::pmr::pool_options options;
options.max_blocks_per_chunk = 1024; // You can suggest chunk size
std::pmr::unsynchronized_pool_resource pool(options);
*/
  //}

  template <typename... Args> T *create(Args &&...args) {
    void *mem = pool.allocate(sizeof(T), alignof(T));
    return new (mem) T(std::forward<Args>(args)...);
  }

  void destroy(T *order) {
    order->~T();
    pool.deallocate(order, sizeof(T), alignof(T));
  }

  /*
void warm_memory(size_t total_bytes) {
std::vector<char> buffer(total_bytes);

// Touch every 4KB (OS page) to fault in the page
// and every 64 bytes (CPU cache line) to fill the cache.
for (size_t i = 0; i < total_bytes; i += 64) {
buffer[i] = 0;
}
}
  */

private:
  Resource pool;
};
