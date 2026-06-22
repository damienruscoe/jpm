#pragma once

#include <cnl/scaled_integer.h>
#include <string_view>

template <int DecimalPlaces, typename BaseTypeT = int64_t> class FixedPointCNL {
private:
  using cnl_fp = cnl::scaled_integer<BaseTypeT, cnl::power<-DecimalPlaces, 10>>;

public:
  using BaseType = BaseTypeT;

  FixedPointCNL() : value_(0) {}
  explicit FixedPointCNL(BaseType raw)
      : value_(cnl::from_rep<cnl_fp, BaseType>{}(raw)) {}
  explicit FixedPointCNL(cnl_fp val) : value_(val) {}

  bool operator==(const FixedPointCNL &) const = default;
  bool operator!=(const FixedPointCNL &) const = default;

  auto operator<=>(const FixedPointCNL &fp_rhs) const {
    auto lhs = cnl::to_rep<cnl_fp>{}(value_);
    auto rhs = cnl::to_rep<cnl_fp>{}(fp_rhs.value_);
    return lhs <=> rhs;
  }

  friend FixedPointCNL operator-(const FixedPointCNL &lhs,
                                 const FixedPointCNL &rhs) {
    return FixedPointCNL{lhs.value_ - rhs.value_};
  }

  static FixedPointCNL Parse(std::string_view str) {
    return ParseFP<FixedPointCNL, DecimalPlaces>(str);
  }

  friend std::ostream &operator<<(std::ostream &os, const FixedPointCNL &fp) {
    auto v = cnl::to_rep<cnl_fp>{}(fp.value_);
    return StreamFP<BaseType, DecimalPlaces>(os, v);
  }

private:
  cnl_fp value_;
};
