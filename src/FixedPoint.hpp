#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string_view>

/**
 * @brief A simple fixed-point decimal representation.
 * For this exercise, we assume a precision of 4 decimal places.
 */
class FixedPoint {
public:
  FixedPoint() : raw_value(0) {}
  explicit FixedPoint(int64_t raw) : raw_value(raw) {}

  static FixedPoint Parse(std::string_view str) {
    if (str.empty()) throw std::invalid_argument("empty");
    size_t dot_pos = str.find('.');
    if (dot_pos == std::string_view::npos) {
      int64_t v = 0;
      auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), v);
      if (ec != std::errc{} || ptr != str.data() + str.size()) throw std::invalid_argument("invalid");
      return FixedPoint(v * 10000);
    } else {
      int64_t integral = 0;
      auto [ptr_i, ec_i] = std::from_chars(str.data(), str.data() + dot_pos, integral);
      if (ec_i != std::errc{} && dot_pos != 0) throw std::invalid_argument("invalid");
      
      std::string_view frac_part = str.substr(dot_pos + 1);
      if (frac_part.empty()) throw std::invalid_argument("invalid");
      int64_t fractional = 0;
      auto [ptr, ec] = std::from_chars(frac_part.data(), frac_part.data() + frac_part.size(), fractional);
      if (ec != std::errc{} || ptr != frac_part.data() + frac_part.size()) throw std::invalid_argument("invalid");
      
      size_t digits = ptr - frac_part.data();
      for (size_t i = digits; i < 4; ++i) fractional *= 10;
      for (size_t i = 4; i < digits; ++i) fractional /= 10;

      return FixedPoint(integral * 10000 + (integral >= 0 ? fractional : -fractional));
    }
  }

  int64_t GetRaw() const { return raw_value; }
  double ToDouble() const { return static_cast<double>(raw_value) / 10000.0; }

private:
  int64_t raw_value;
};
