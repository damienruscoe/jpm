#pragma once
#include <cassert>
#include <limits>
#include <vector>

namespace siv {

using ID = uint64_t;
static constexpr size_t InvalidID = std::numeric_limits<size_t>::max();

template <typename T> class Vector {
public:
  Vector() = default;

  size_t push_back(const T &object) {
    const size_t id = getFreeSlot();
    m_data.push_back(object);
    return id;
  }

  template <typename... TArgs> size_t emplace_back(TArgs &&...args) {
    const size_t id = getFreeSlot();
    m_data.emplace_back(std::forward<TArgs>(args)...);
    return id;
  }

  void erase(size_t id) {
    const size_t data_id = m_indexes[id];
    const size_t last_data_id = m_data.size() - 1;
    const size_t last_id = m_metadata[last_data_id];

    std::swap(m_data[data_id], m_data[last_data_id]);
    std::swap(m_metadata[data_id], m_metadata[last_data_id]);
    std::swap(m_indexes[id], m_indexes[last_id]);

    m_data.pop_back();
  }

  T &operator[](size_t id) { return m_data[m_indexes[id]]; }
  T const &operator[](size_t id) const { return m_data[m_indexes[id]]; }

  size_t size() const { return m_data.size(); }
  bool empty() const { return m_data.empty(); }

  T *get(size_t id) {
    if (id < m_indexes.size() && m_indexes[id] < m_data.size())
      return &m_data[m_indexes[id]];
    return nullptr;
  }

  const T *get(size_t id) const {
    if (id < m_indexes.size() && m_indexes[id] < m_data.size())
      return &m_data[m_indexes[id]];
    return nullptr;
  }

private:
  size_t getFreeSlot() {
    auto data_size = m_data.size();
    size_t id;

    if (m_metadata.size() > data_size) {
      // Reusing an ID: This ID might be larger than current m_indexes,
      // so resize is potentially necessary.
      id = m_metadata[data_size];
      if (id >= m_indexes.size()) {
        m_indexes.resize(id + 1);
      }
      m_indexes[id] = data_size;
    } else {
      // Allocating a new ID: new_id is exactly data_size,
      // which is guaranteed to be <= m_indexes.size() because
      // m_indexes is maintained in lock-step with m_metadata/m_data.
      id = data_size;
      m_metadata.push_back(id);
      m_indexes.push_back(id);
      m_indexes[id] = data_size;
    }

    return id;
  }

  std::vector<size_t> m_metadata;
  std::vector<size_t> m_indexes;
  std::vector<T> m_data;
};
} // namespace siv
