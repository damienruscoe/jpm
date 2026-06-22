#pragma once
#include "object_pool.hpp"
#include <string>
#include <string_view>
#include <unordered_set>

template <typename ID, typename T> struct SetTraits {
  struct Hasher {
    using is_transparent = void;
    size_t operator()(const T *ptr) const { return std::hash<ID>{}(ptr->id); }
    size_t operator()(const ID &id) const { return std::hash<ID>{}(id); }
  };

  struct KeyEqual {
    using is_transparent = void;
    bool operator()(const T *lhs, const T *rhs) const {
      return lhs->id == rhs->id;
    }
    bool operator()(const T *lhs, const ID &rhs) const {
      return lhs->id == rhs;
    }
    bool operator()(const ID &lhs, const T *rhs) const {
      return lhs == rhs->id;
    }
  };
};

template <typename T> struct SetTraits<std::string, T> {
  struct Hasher {
    using is_transparent = void;
    size_t operator()(const T *ptr) const {
      return std::hash<std::string>{}(ptr->id);
    }
    size_t operator()(const std::string &s) const {
      return std::hash<std::string>{}(s);
    }
    size_t operator()(std::string_view sv) const {
      return std::hash<std::string_view>{}(sv);
    }
    size_t operator()(const char *s) const {
      return std::hash<std::string_view>{}(s);
    }
  };

  struct KeyEqual {
    using is_transparent = void;
    bool operator()(const T *lhs, const T *rhs) const {
      return lhs->id == rhs->id;
    }
    bool operator()(const T *lhs, const std::string &rhs) const {
      return lhs->id == rhs;
    }
    bool operator()(const std::string &lhs, const T *rhs) const {
      return lhs == rhs->id;
    }
    bool operator()(const T *lhs, std::string_view rhs) const {
      return lhs->id == rhs;
    }
    bool operator()(std::string_view lhs, const T *rhs) const {
      return lhs == rhs->id;
    }
    bool operator()(const T *lhs, const char *rhs) const {
      return lhs->id == rhs;
    }
    bool operator()(const char *lhs, const T *rhs) const {
      return lhs == rhs->id;
    }
  };
};

template <typename ID, typename T> struct ObjectResource {
public:
  using key_type = ID;
  using value_type = T;

  template <typename... Args> T *create(Args &&...args) {
    T *order = m_storage.create(std::forward<Args>(args)...);
    m_id_set.insert(order);
    return order;
  }

  template <typename K> void erase(K &&key) {
    if (auto it = m_id_set.find(std::forward<K>(key)); it != m_id_set.end()) {
      T *order = *it;
      m_id_set.erase(it);
      m_storage.destroy(order);
    }
  }

  template <typename K> [[nodiscard]] bool contains(K &&key) const {
    return m_id_set.find(std::forward<K>(key)) != m_id_set.end();
  }

  template <typename K> T *find(K &&key) const {
    auto it = m_id_set.find(std::forward<K>(key));
    return it != m_id_set.end() ? *it : nullptr;
  }

private:
  ObjectPool<T> m_storage;
  std::unordered_set<T *, typename SetTraits<ID, T>::Hasher,
                     typename SetTraits<ID, T>::KeyEqual>
      m_id_set;
};
