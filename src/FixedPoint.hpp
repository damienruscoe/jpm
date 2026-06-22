#pragma once

#include <string_view>

template <int DecimalPlaces, typename BaseTypeT = int64_t> class FixedPointCustom {
public:
  using BaseType = BaseTypeT;

  FixedPointCustom() : m_raw_value(0) {}
  explicit FixedPointCustom(BaseType raw) : m_raw_value(raw) {}

  bool operator==(const FixedPointCustom &) const = default;
  bool operator!=(const FixedPointCustom &) const = default;

  auto operator<=>(const FixedPointCustom &) const = default;

  friend FixedPointCustom operator-(const FixedPointCustom &lhs, const FixedPointCustom &rhs) {
    return FixedPointCustom{lhs.m_raw_value - rhs.m_raw_value};
  }

  static FixedPointCustom Parse(std::string_view str) {
    return ParseFP<FixedPointCustom, DecimalPlaces>(str);
  }

  friend std::ostream &operator<<(std::ostream &os, const FixedPointCustom &fp) {
    return StreamFP<BaseType, DecimalPlaces>(os, fp.m_raw_value);
  }

private:
  BaseType m_raw_value;
};
