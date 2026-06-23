#pragma once

#include <charconv>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include <cnl/scaled_integer.h>

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

template <int DecimalPlaces, typename BaseTypeT = int64_t> 
class FixedPoint {
private:
  using cnl_fp = cnl::scaled_integer<BaseTypeT, cnl::power<-DecimalPlaces, 10>>;

public:
  using BaseType = BaseTypeT;
	static constexpr int decimals = DecimalPlaces;

  FixedPoint() : m_value(0) {}
  explicit FixedPoint(BaseType raw)
      : m_value(cnl::from_rep<cnl_fp, BaseType>{}(raw * std::pow(10, DecimalPlaces))) {}

	friend std::ostream &operator<<(std::ostream &os, const FixedPoint &fp) {
		auto v = fp.get_raw_value();
		if (v < 0) {
			os << '-';
			v = -v;
		}

		const uint64_t foo = std::pow(10, DecimalPlaces);
		return os << (v / foo) << '.' << std::setfill('0') << std::setw(DecimalPlaces)
							<< (v % foo);
	}

	friend FixedPoint operator-(const FixedPoint &lhs, const FixedPoint &rhs) {
			return FixedPoint{lhs.get_raw_value() - rhs.get_raw_value(), true};
	}

	friend bool operator==(const FixedPoint& lhs, const FixedPoint& rhs) {
			return lhs.get_raw_value() == rhs.get_raw_value();
	}

	friend auto operator<=>(const FixedPoint& lhs, const FixedPoint& rhs) {
			return lhs.get_raw_value() <=> rhs.get_raw_value();
	}

	static FixedPoint Parse(std::string_view str) {
			return FixedPoint(ParseFP_Impl<BaseType, decimals>(str), true);
	}

private:
  FixedPoint(cnl_fp val) : m_value(val) {}
  FixedPoint(BaseType raw, bool)
      : m_value(cnl::from_rep<cnl_fp, BaseType>{}(raw)) {}

  BaseType get_raw_value() const { return cnl::to_rep<cnl_fp>{}(m_value); }

  cnl_fp m_value;
};

