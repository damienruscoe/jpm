#pragma once

#include <charconv>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>

template <typename BaseType, int DecimalPlaces>
static inline BaseType ParseFP_Impl(std::string_view str) {
  if (str.empty())
    throw std::invalid_argument("empty");

  bool negative = false;
  if (str[0] == '-') {
    negative = true;
    str.remove_prefix(1);
  } else if (str[0] == '+') {
    str.remove_prefix(1);
  }

  if (str.empty())
    throw std::invalid_argument("invalid");

  BaseType integral = 0;
  BaseType fractional = 0;

  size_t dot_pos = str.find('.');
  if (dot_pos == std::string_view::npos) {

    auto [ptr, ec] =
        std::from_chars(str.data(), str.data() + str.size(), integral);
    if (ec != std::errc{} || ptr != str.data() + str.size())
      throw std::invalid_argument("invalid");
  } else {
    if (dot_pos == 0 || dot_pos == str.size() - 1)
      throw std::invalid_argument("invalid");

    auto [ptr_i, ec_i] =
        std::from_chars(str.data(), str.data() + dot_pos, integral);
    if (ec_i != std::errc{})
      throw std::invalid_argument("invalid");

    std::string_view frac_part = str.substr(dot_pos + 1);

    auto [ptr_f, ec_f] = std::from_chars(
        frac_part.data(), frac_part.data() + frac_part.size(), fractional);
    if (ec_f != std::errc{} || ptr_f != frac_part.data() + frac_part.size())
      throw std::invalid_argument("invalid");

    size_t digits = ptr_f - frac_part.data();
    for (size_t i = digits; i < DecimalPlaces; ++i)
      fractional *= 10;
    for (size_t i = DecimalPlaces; i < digits; ++i)
      fractional /= 10;
  }

  const uint64_t foo = (uint64_t)std::pow(10, DecimalPlaces);

  BaseType val = (integral * foo) + (fractional);
  return negative ? -val : val;
}

template <typename FP, int DecimalPlaces>
static inline FP ParseFP(std::string_view str) {
  return FP(ParseFP_Impl<typename FP::BaseType, DecimalPlaces>(str));
}

template <typename BaseType, int DecimalPlaces>
static inline std::ostream &StreamFP(std::ostream &os, BaseType v) {
  if (v < 0) {
    os << '-';
    v = -v;
  }

  const uint64_t foo = std::pow(10, DecimalPlaces);
  return os << (v / foo) << '.' << std::setfill('0') << std::setw(DecimalPlaces)
            << (v % foo);
}

#ifdef USE_BOOST_FP
#include "FixedPointCNL.hpp"
template <int N, typename T = uint64_t> using FixedPoint = FixedPointCNL<N, T>;
using FixedPointGeneric = FixedPoint<4>;
#else
#include "FixedPoint.hpp"
using FixedPointGeneric = FixedPoint<4>;
#endif
