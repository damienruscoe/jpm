#include "object_pool.hpp"
#include <unordered_map>

template <typename ID, typename T> struct ObjectResource {
public:
  using key_type = ID;
  using value_type = T;

  template <typename... Args> T *create(ID id, Args &&...args) {
    if (m_id_map.find(id) != m_id_map.end())
      return nullptr; // TODO: Is this the correct interface?
    T *order = m_storage.create(std::forward<Args>(args)...);
    m_id_map[id] = order;
    return order;
  }

  void erase(T &order) {
    m_id_map.erase(order.id);
    m_storage.destroy(&order);
  };

  void erase(ID &id) {
    if (auto it = m_id_map.find(id); it != m_id_map.end())
      erase(it->second);
  };

  [[nodiscard]] bool contains(ID id) const {
    return m_id_map.find(id) != m_id_map.end();
  }

  T *find(ID id) const {
    auto it = m_id_map.find(id);
    return it != m_id_map.end() ? it->second : nullptr;
  }

private:
  ObjectPool<T> m_storage;
  std::unordered_map<ID, T *> m_id_map;
};
