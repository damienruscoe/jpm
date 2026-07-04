#pragma once
#include "object_pool.hpp"
#include <concepts>
#include <functional>
#include <string_view>
#include <type_traits>
#include <unordered_set>

template <typename K>
concept StringViewNormalizable =
    std::convertible_to<K, std::string_view> &&
    !std::same_as<std::decay_t<K>, std::string_view>;

template <typename T, typename KeyGetter> class ObjectResource {
public:
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

  auto begin() const { return m_id_set.begin(); }

  auto end() const { return m_id_set.end(); }

private:
  ObjectPool<T> m_storage;

  struct Hash {
    using is_transparent = void;

    template <typename K>
    using normalized_t =
        std::conditional_t<StringViewNormalizable<K>, std::string_view, K>;

    size_t operator()(T *p) const {
      return std::hash<normalized_t<decltype(KeyGetter::get(p))>>{}(
          KeyGetter::get(p));
    }

    template <typename K> size_t operator()(const K &k) const {
      return std::hash<normalized_t<K>>{}(k);
    }
  };

  struct Equal {
    using is_transparent = void;

    bool operator()(T *a, T *b) const {
      return KeyGetter::get(a) == KeyGetter::get(b);
    }

    template <typename K> bool operator()(T *a, const K &k) const {
      return KeyGetter::get(a) == k;
    }

    template <typename K> bool operator()(const K &k, T *a) const {
      return k == KeyGetter::get(a);
    }
  };

  std::unordered_set<T *, Hash, Equal> m_id_set;
};
