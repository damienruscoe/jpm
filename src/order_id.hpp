#pragma once
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string_view>

struct FixedSizeOrderID {
  char data[10];

  FixedSizeOrderID() { std::memset(data, 0, sizeof(data)); }
  FixedSizeOrderID(std::string_view sv) {
    std::memset(data, 0, sizeof(data));
    std::memcpy(data, sv.data(), std::min(sv.size(), sizeof(data)));
  }

  bool operator==(const FixedSizeOrderID &other) const {
    return std::memcmp(data, other.data, sizeof(data)) == 0;
  }

  bool operator==(std::string_view sv) const {
    return static_cast<std::string_view>(*this) == sv;
  }

  operator std::string_view() const {
    return {data, strnlen(data, sizeof(data))};
  }
};

namespace std {
template <> struct hash<FixedSizeOrderID> {
  using is_transparent = void;
  size_t operator()(const FixedSizeOrderID &id) const {
    return std::hash<std::string_view>{}(id);
  }
  size_t operator()(std::string_view sv) const {
    return std::hash<std::string_view>{}(sv);
  }
};
} // namespace std

inline std::ostream &operator<<(std::ostream &os, const FixedSizeOrderID &id) {
  return os << static_cast<std::string_view>(id);
}

inline bool operator==(std::string_view sv, const FixedSizeOrderID &id) {
  return id == sv;
}
