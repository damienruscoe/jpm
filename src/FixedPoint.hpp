#pragma once

#include <string_view>

template <int DecimalPlaces, typename BaseTypeT = int64_t> class FixedPoint {
public:
  using BaseType = BaseTypeT;

  FixedPoint() : m_raw_value(0) {}
  explicit FixedPoint(BaseType raw) : m_raw_value(raw) {}

  bool operator==(const FixedPoint &) const = default;
  bool operator!=(const FixedPoint &) const = default;

  auto operator<=>(const FixedPoint &) const = default;

  friend FixedPoint operator-(const FixedPoint &lhs, const FixedPoint &rhs) {
    return FixedPoint{lhs.m_raw_value - rhs.m_raw_value};
  }

  static FixedPoint Parse(std::string_view str) {
    return ParseFP<FixedPoint, DecimalPlaces>(str);
  }

  friend std::ostream &operator<<(std::ostream &os, const FixedPoint &fp) {
    return StreamFP<BaseType, DecimalPlaces>(os, fp.m_raw_value);
  }

private:
  BaseType m_raw_value;
};
