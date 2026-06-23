#pragma once

#include "str_utils.hpp"
#include <charconv>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>

#include <cnl/scaled_integer.h>

namespace {

template <typename BaseType, int DecimalPlaces>
static inline std::optional<BaseType> parse_float(std::string_view str);

}

template <int DecimalPlaces, typename BaseTypeT = int64_t> class FixedPoint {
private:
  using cnl_fp = cnl::scaled_integer<BaseTypeT, cnl::power<-DecimalPlaces, 10>>;

public:
  using BaseType = BaseTypeT;
  static constexpr int decimals = DecimalPlaces;

  FixedPoint() : m_value(0) {}
  explicit FixedPoint(BaseType raw)
      : m_value(cnl::from_rep<cnl_fp, BaseType>{}(
            raw * std::pow(10, DecimalPlaces))) {}

  friend std::ostream &operator<<(std::ostream &os, const FixedPoint &fp) {
    auto v = fp.get_raw_value();
    if (v < 0) {
      os << '-';
      v = -v;
    }

    const uint64_t foo = std::pow(10, DecimalPlaces);
    return os << (v / foo) << '.' << std::setfill('0')
              << std::setw(DecimalPlaces) << (v % foo);
  }

  friend FixedPoint operator-(const FixedPoint &lhs, const FixedPoint &rhs) {
    return FixedPoint{lhs.get_raw_value() - rhs.get_raw_value(), true};
  }

  friend bool operator==(const FixedPoint &lhs, const FixedPoint &rhs) {
    return lhs.get_raw_value() == rhs.get_raw_value();
  }

  friend auto operator<=>(const FixedPoint &lhs, const FixedPoint &rhs) {
    return lhs.get_raw_value() <=> rhs.get_raw_value();
  }

  static std::optional<FixedPoint> Parse(std::string_view str) {
    if (auto result = parse_float<BaseType, decimals>(str))
      return {FixedPoint(*result, true)};
    return std::nullopt;
  }

private:
  FixedPoint(cnl_fp val) : m_value(val) {}
  FixedPoint(BaseType raw, bool)
      : m_value(cnl::from_rep<cnl_fp, BaseType>{}(raw)) {}

  BaseType get_raw_value() const { return cnl::to_rep<cnl_fp>{}(m_value); }

  cnl_fp m_value;
};

namespace {

template <typename T> auto parse_int(std::string_view str) {
  T result{};

  if (str.empty() || str.front() == '-')
    return std::make_pair(result, false);

  const bool success = parse_integer(str, result);
  return std::make_pair(result, success);
}

template <typename BaseType, int DecimalPlaces>
static inline std::optional<BaseType> parse_float(std::string_view str) {
  if (str.empty())
    return std::nullopt;

  std::string_view whole_part = str;

  const size_t dot_pos = str.find('.');
  if (dot_pos != std::string_view::npos)
    whole_part = whole_part.substr(0, dot_pos);

  // Trim the sign from the whole part number
  const bool negative = whole_part.front() == '-';
  if (negative || whole_part.front() == '+')
    whole_part.remove_prefix(1);

  auto [result, success] = parse_int<BaseType>(whole_part);
  if (!success)
    return std::nullopt;

  if constexpr (DecimalPlaces > 0) {
    [[maybe_unused]] auto decimal_adjust = [](const BaseType &val,
                                              unsigned adj = 0) {
      return val * (uint64_t)std::pow(10, DecimalPlaces - adj);
    };

    result = decimal_adjust(result);

    if (dot_pos != std::string_view::npos) {
      std::string_view frac_part = str.substr(dot_pos + 1);
      frac_part = frac_part.substr(0, DecimalPlaces);

      auto [fractional, success] = parse_int<BaseType>(frac_part);
      if (!success)
        return std::nullopt;
      result += decimal_adjust(fractional, frac_part.size());
    }
  }

  return negative ? -result : result;
}

} // namespace
