#pragma once
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <string_view>

struct FixedSizeOrderID {
  std::array<char, 10> data{};

  FixedSizeOrderID() = default;
  FixedSizeOrderID(std::string_view sv) {
    std::memcpy(data.data(), sv.data(), std::min(sv.size(), data.size()));
  }

  bool operator==(const FixedSizeOrderID &other) const {
    return data == other.data;
  }

  bool operator==(std::string_view sv) const {
    return static_cast<std::string_view>(*this) == sv;
  }

  operator std::string_view() const {
    return {data.data(), strnlen(data.data(), data.size())};
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
