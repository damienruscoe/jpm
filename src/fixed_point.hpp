#pragma once

#include <array>
#include <charconv>
#include <iomanip>
#include <sstream>

// clang-format off
constexpr auto Denom = std::to_array<unsigned long long>({
	1,
	10,
	100,
	1'000,
	10'000,
	100'000,
	1'000'000,
	10'000'000,
	100'000'000,
	1'000'000'000,
	10'000'000'000,
	100'000'000'000,
	1'000'000'000'000
});
// clang-format on

template <int Exponent, typename Value> class FixedPoint;

template <int Exponent, typename Value>
std::ostream &operator<<(std::ostream &os,
                         const FixedPoint<Exponent, Value> &fixed_point);

template <int Exponent, typename Value = uint32_t> class FixedPoint {
public:
  static_assert(Exponent < Denom.size() - 1);

  static constexpr int exponent = Exponent;
  using value_t = Value;

  FixedPoint(Value v = 0) : v(v) {}

  auto operator<=>(const FixedPoint &) const = default;
  bool operator==(const FixedPoint &) const = default;

  operator bool() const;

  FixedPoint operator-(const FixedPoint &rhs) const;
  FixedPoint &operator-=(const FixedPoint &rhs);

  FixedPoint operator+(const FixedPoint &rhs) const;
  FixedPoint &operator+=(const FixedPoint &rhs);

  friend std::ostream &operator<< <>(std::ostream &,
                                     const FixedPoint<Exponent, Value> &);

  std::string to_str() const;
  double to_double() const;

  static FixedPoint Parse(std::string_view str);

private:
  Value v;
};

template <int Exponent, typename Value>
FixedPoint<Exponent, Value>::operator bool() const {
  return v != 0;
}

template <int Exponent, typename Value>
FixedPoint<Exponent, Value> FixedPoint<Exponent, Value>::operator-(
    const FixedPoint<Exponent, Value> &rhs) const {
  return v - rhs.v;
}

template <int Exponent, typename Value>
FixedPoint<Exponent, Value> &FixedPoint<Exponent, Value>::operator-=(
    const FixedPoint<Exponent, Value> &rhs) {
  v -= rhs.v;
  return *this;
}

template <int Exponent, typename Value>
FixedPoint<Exponent, Value> FixedPoint<Exponent, Value>::operator+(
    const FixedPoint<Exponent, Value> &rhs) const {
  return v + rhs.v;
}

template <int Exponent, typename Value>
FixedPoint<Exponent, Value> &FixedPoint<Exponent, Value>::operator+=(
    const FixedPoint<Exponent, Value> &rhs) {
  v += rhs.v;
  return *this;
}

template <int Exponent, typename Value>
std::ostream &operator<<(std::ostream &os,
                         const FixedPoint<Exponent, Value> &fixed_point) {
  if constexpr (Exponent == 0) {
    return os << fixed_point.v;
  } else {
    const auto &divisor = Denom[Exponent];
    const auto whole = fixed_point.v / divisor;
    const auto fractional = fixed_point.v % divisor;

    std::ios state(nullptr);
    state.copyfmt(os);

    os << whole << "." << std::setfill('0') << std::setw(Exponent)
       << fractional;

    os.copyfmt(state);
    return os;
  }
}

template <int Exponent, typename Value>
std::string FixedPoint<Exponent, Value>::to_str() const {
  std::ostringstream ss;
  ss << *this;
  return ss.str();
}

template <int Exponent, typename Value>
double FixedPoint<Exponent, Value>::to_double() const {
  const auto &divisor = Denom[Exponent];
  return (double)v / divisor;
}

template <int Exponent, typename Value>
FixedPoint<Exponent, Value>
FixedPoint<Exponent, Value>::Parse(std::string_view str) {
  Value x{0}, y{0};
  const char *end = str.data() + str.size();

  auto [ptr, err] = std::from_chars(str.data(), end, x);

  if (ptr != str.data() + str.size() && *ptr == '.') {
    const char *frac_start = ++ptr;
    const char *frac_end = std::min(frac_start + Exponent, end);

    auto [ptr2, err2] = std::from_chars(frac_start, frac_end, y);

    size_t digits_read = ptr2 - frac_start;
    if (digits_read < Exponent)
      y *= Denom[Exponent - digits_read];
  }

  return FixedPoint(x * Denom[Exponent] + y);
}
