#pragma once
#include "object_pool.hpp"
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

// Generic default traits
template <typename ID> struct MapTraits {
  using Hasher = std::hash<ID>;
  using KeyEqual = std::equal_to<ID>;
};

// Specialization for std::string to enable transparent lookup
template <> struct MapTraits<std::string> {
  struct Hasher {
    using is_transparent = void;
    size_t operator()(const std::string &s) const {
      return std::hash<std::string>{}(s);
    }
    size_t operator()(std::string_view sv) const {
      return std::hash<std::string_view>{}(sv);
    }
  };
  using KeyEqual = std::equal_to<>;
  /*
struct KeyEqual {
using is_transparent = void;
bool operator()(const std::string &lhs, const std::string &rhs) const {
return lhs == rhs;
}
bool operator()(const std::string &lhs, std::string_view rhs) const {
return lhs == rhs;
}
bool operator()(std::string_view lhs, const std::string &rhs) const {
return lhs == rhs;
}
};
  */
};

template <typename ID, typename T> struct ObjectResource {
public:
  using key_type = ID;
  using value_type = T;

  template <typename... Args> T *create(std::string_view id, Args &&...args) {
    if (m_id_map.find(id) != m_id_map.end())
      return nullptr;
    T *order = m_storage.create(std::forward<Args>(args)...);
    m_id_map[ID(id)] = order; // ID conversion here!
    return order;
  }

  void erase(T &order) {
    m_id_map.erase(order.id);
    m_storage.destroy(&order);
  };

  void erase(std::string_view id) {
    if (auto it = m_id_map.find(id); it != m_id_map.end())
      erase(*it->second);
  };

  [[nodiscard]] bool contains(std::string_view id_sv) const {
    return m_id_map.find(id_sv) != m_id_map.end();
  }

  T *find(std::string_view id_sv) const {
    auto it = m_id_map.find(id_sv);
    return it != m_id_map.end() ? it->second : nullptr;
  }

private:
  ObjectPool<T> m_storage;
  std::unordered_map<ID, T *, typename MapTraits<ID>::Hasher,
                     typename MapTraits<ID>::KeyEqual>
      m_id_map;
};
