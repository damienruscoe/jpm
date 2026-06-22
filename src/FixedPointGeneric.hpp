#pragma once

#ifdef USE_BOOST_FP
#include <cnl/scaled_integer.h>
#include <string>
#include <string_view>
#include <stdexcept>
#include <iostream>
#include <charconv>

// Represents int64_t scaled by 2^-4, equivalent to 4 decimal places of binary precision.
using cnl_fp = cnl::scaled_integer<int64_t, cnl::power<-4>>;

class FixedPointGeneric {
    cnl_fp value_;
public:
    FixedPointGeneric() : value_(0) {}
    // Construct by casting raw int64_t to the representation type
    explicit FixedPointGeneric(int64_t raw) : value_(static_cast<cnl_fp>(raw)) {}
    explicit FixedPointGeneric(cnl_fp val) : value_(val) {}

    static FixedPointGeneric Parse(std::string_view str) {
        if (str.empty()) throw std::invalid_argument("empty");

        bool negative = false;
        size_t start = 0;
        if (str[0] == '-') { negative = true; start = 1; }
        else if (str[0] == '+') { start = 1; }

        if (start == str.size()) throw std::invalid_argument("invalid");

        size_t dot_pos = str.find('.');
        int64_t integral = 0;
        int64_t fractional = 0;

        if (dot_pos == std::string_view::npos) {
            auto [ptr, ec] = std::from_chars(str.data() + start, str.data() + str.size(), integral);
            if (ec != std::errc{} || ptr != str.data() + str.size()) throw std::invalid_argument("invalid");
        } else {
            if (dot_pos == start || dot_pos == str.size() - 1) throw std::invalid_argument("invalid");
            
            auto [ptr_i, ec_i] = std::from_chars(str.data() + start, str.data() + dot_pos, integral);
            if (ec_i != std::errc{}) throw std::invalid_argument("invalid");
            
            std::string_view frac_part = str.substr(dot_pos + 1);
            auto [ptr_f, ec_f] = std::from_chars(frac_part.data(), frac_part.data() + frac_part.size(), fractional);
            if (ec_f != std::errc{} || ptr_f != frac_part.data() + frac_part.size()) throw std::invalid_argument("invalid");
            
            size_t digits = frac_part.size();
            for (size_t i = digits; i < 4; ++i) fractional *= 10;
            for (size_t i = 4; i < digits; ++i) fractional /= 10;
        }

        int64_t val = integral * 10000 + fractional;
        return FixedPointGeneric(negative ? -val : val);
    }

    double ToDouble() const { return static_cast<double>(static_cast<int64_t>(value_)) / 10000.0; }
    int64_t GetRaw() const { return static_cast<int64_t>(value_); }

    bool operator==(const FixedPointGeneric &other) const { return value_ == other.value_; }
    bool operator<(const FixedPointGeneric &other) const { return value_ < other.value_; }
    bool operator>(const FixedPointGeneric &other) const { return value_ > other.value_; }
    bool operator<=(const FixedPointGeneric &other) const { return value_ <= other.value_; }
    bool operator>=(const FixedPointGeneric &other) const { return value_ >= other.value_; }

    friend FixedPointGeneric operator-(const FixedPointGeneric &lhs, const FixedPointGeneric &rhs) {
        return FixedPointGeneric(static_cast<int64_t>(lhs.value_ - rhs.value_));
    }
    
    friend std::ostream &operator<<(std::ostream &os, const FixedPointGeneric &fp) {
        return os << fp.ToDouble();
    }
};

#else
#include "FixedPoint.hpp"
using FixedPointGeneric = FixedPointAI;
#endif
