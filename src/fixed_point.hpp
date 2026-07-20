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

  static constexpr int64_t Scale = []() {
    int64_t val = 1;
    for (int i = 0; i < DecimalPlaces; ++i)
      val *= 10;
    return val;
  }();

public:
  using BaseType = BaseTypeT;
  static constexpr int decimals = DecimalPlaces;

  FixedPoint() : m_value(0) {}
  /*explicit*/ FixedPoint(BaseType raw)
      : m_value(cnl::from_rep<cnl_fp, BaseType>{}(
            raw * std::pow(10, DecimalPlaces))) {}

  friend std::ostream &operator<<(std::ostream &os, const FixedPoint &fp) {
    std::stringstream ss;
    auto v = fp.get_raw_value();
    if (v < 0) {
      ss << '-';
      v = -v;
    }

    ss << (v / Scale) << '.' << std::setfill('0') << std::setw(DecimalPlaces)
       << (v % Scale);
    return os << ss.str();
  }

  friend bool operator==(const FixedPoint &lhs, const FixedPoint &rhs) {
    return lhs.get_raw_value() == rhs.get_raw_value();
  }

  friend auto operator<=>(const FixedPoint &lhs, const FixedPoint &rhs) {
    return lhs.get_raw_value() <=> rhs.get_raw_value();
  }

  friend FixedPoint operator-(const FixedPoint &negate) {
    return FixedPoint{-negate.get_raw_value()};
  }

  friend FixedPoint operator-(const FixedPoint &lhs, const FixedPoint &rhs) {
    return FixedPoint{lhs.get_raw_value() - rhs.get_raw_value(), true};
  }

  friend FixedPoint operator+(const FixedPoint &lhs, const FixedPoint &rhs) {
    return FixedPoint{lhs.get_raw_value() + rhs.get_raw_value(), true};
  }

  friend FixedPoint operator*(const FixedPoint &lhs, const FixedPoint &rhs) {
    // const auto Scale = std::pow(10, DecimalPlaces);
    __int128_t intermediate =
        static_cast<__int128_t>(lhs.get_raw_value()) * rhs.get_raw_value();
    __int128_t raw_product = intermediate / Scale;
    return FixedPoint{static_cast<int64_t>(raw_product), true};
  }

  friend FixedPoint operator/(const FixedPoint &lhs, const FixedPoint &rhs) {
    // const auto Scale = std::pow(10, DecimalPlaces);
    __int128_t numerator = static_cast<__int128_t>(lhs.get_raw_value()) * Scale;
    __int128_t raw_quotient = numerator / rhs.get_raw_value();
    return FixedPoint{static_cast<int64_t>(raw_quotient), true};
  }

  FixedPoint &operator-=(const FixedPoint &rhs) {
    *this = *this - rhs;
    return *this;
  }

  FixedPoint &operator+=(const FixedPoint &rhs) {
    *this = *this + rhs;
    return *this;
  }

  FixedPoint &operator*=(const FixedPoint &rhs) {
    *this = *this * rhs;
    return *this;
  }

  FixedPoint &operator/=(const FixedPoint &rhs) {
    *this = *this / rhs;
    return *this;
  }

  explicit operator bool() const { return get_raw_value() != 0; }

  explicit operator double() const {
    return static_cast<double>(get_raw_value()) / Scale;
  }

  static std::optional<FixedPoint> Parse(std::string_view str) {
    if (auto result = parse_float<BaseType, decimals>(str))
      return {FixedPoint(*result, true)};
    return std::nullopt;
  }

  static FixedPoint Parsed(BaseType value) { return FixedPoint(value, true); }

  friend std::hash<FixedPoint<DecimalPlaces, BaseTypeT>>;

private:
  FixedPoint(cnl_fp val) : m_value(val) {}
  FixedPoint(BaseType raw, bool)
      : m_value(cnl::from_rep<cnl_fp, BaseType>{}(raw)) {}

  BaseType get_raw_value() const { return cnl::to_rep<cnl_fp>{}(m_value); }

  cnl_fp m_value;
};

namespace std {
template <int N, typename BaseTypeT> struct hash<FixedPoint<N, BaseTypeT>> {
  std::size_t operator()(const FixedPoint<N, BaseTypeT> &fp) const noexcept {
    return std::hash<uint64_t>{}(fp.get_raw_value());
  }
};
} // namespace std

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
