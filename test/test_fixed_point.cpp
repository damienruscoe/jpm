#include "../src/fixed_point.hpp"
#include <gtest/gtest.h>
#include <string>

class FixedPointTest : public testing::Test {};

static auto to_test_string(const auto &fp) {
  std::stringstream ss;
  ss << fp;
  return ss.str();
}

TEST(FixedPointTest, DefaultInitialized) {
  auto fp = FixedPoint<4>();
  EXPECT_EQ(to_test_string(fp), "0.0000");
}

TEST(FixedPointTest, ValueInitialized) {
  {
    auto fp = FixedPoint<4>(0);
    EXPECT_EQ(to_test_string(fp), "0.0000");
  }
  {
    auto fp = FixedPoint<4>(1);
    EXPECT_EQ(to_test_string(fp), "1.0000");
  }
  {
    auto fp = FixedPoint<4>(2);
    EXPECT_EQ(to_test_string(fp), "2.0000");
  }
  {
    auto fp = FixedPoint<4>(10000);
    EXPECT_EQ(to_test_string(fp), "10000.0000");
  }
  {
    auto fp = FixedPoint<4>(20000);
    EXPECT_EQ(to_test_string(fp), "20000.0000");
  }
  {
    auto fp = FixedPoint<4>(-1);
    EXPECT_EQ(to_test_string(fp), "-1.0000");
  }
  {
    auto fp = FixedPoint<4>(-10);
    EXPECT_EQ(to_test_string(fp), "-10.0000");
  }
}

TEST(FixedPointTest, ParsingVariousPrecisions) {
  {
    auto fp = *FixedPoint<0>::Parse("1.12345");
    EXPECT_EQ(to_test_string(fp), "1.0");
  }
  {
    auto fp = *FixedPoint<1>::Parse("1.12345");
    EXPECT_EQ(to_test_string(fp), "1.1");
  }
  {
    auto fp = *FixedPoint<2>::Parse("1.12345");
    EXPECT_EQ(to_test_string(fp), "1.12");
  }
  {
    auto fp = *FixedPoint<3>::Parse("1.12345");
    EXPECT_EQ(to_test_string(fp), "1.123");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1.12345");
    EXPECT_EQ(to_test_string(fp), "1.1234");
  }
  {
    auto fp = *FixedPoint<5>::Parse("1.12345");
    EXPECT_EQ(to_test_string(fp), "1.12345");
  }
  {
    auto fp = *FixedPoint<6>::Parse("1.12345");
    EXPECT_EQ(to_test_string(fp), "1.123450");
  }
  {
    auto fp = *FixedPoint<7>::Parse("1.12345");
    EXPECT_EQ(to_test_string(fp), "1.1234500");
  }
}

TEST(FixedPointTest, Parsing) {
  {
    auto fp = *FixedPoint<4>::Parse("0");
    EXPECT_EQ(to_test_string(fp), "0.0000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1");
    EXPECT_EQ(to_test_string(fp), "1.0000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("-1");
    EXPECT_EQ(to_test_string(fp), "-1.0000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("+1");
    EXPECT_EQ(to_test_string(fp), "1.0000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1.1");
    EXPECT_EQ(to_test_string(fp), "1.1000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1.5000000");
    EXPECT_EQ(to_test_string(fp), "1.5000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("-1.5000000");
    EXPECT_EQ(to_test_string(fp), "-1.5000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1.0001");
    EXPECT_EQ(to_test_string(fp), "1.0001");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1.00001");
    EXPECT_EQ(to_test_string(fp), "1.0000");
  }
  {
    // No rounding up
    auto fp = *FixedPoint<4>::Parse("0.0000999");
    EXPECT_EQ(to_test_string(fp), "0.0000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("00001");
    EXPECT_EQ(to_test_string(fp), "1.0000");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1.2345");
    EXPECT_EQ(to_test_string(fp), "1.2345");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1.23456");
    EXPECT_EQ(to_test_string(fp), "1.2345");
  }
  {
    auto fp = *FixedPoint<4>::Parse("1.2345_X");
    EXPECT_EQ(to_test_string(fp), "1.2345");
  }
  {
    // Large but valid?
    auto fp = *FixedPoint<4>::Parse("999999999999999999");
    EXPECT_EQ(to_test_string(fp), "186471204942301.4128");
  }
  {
    // Large but valid?
    auto fp = *FixedPoint<4>::Parse("1000000000000000000.0");
    EXPECT_EQ(to_test_string(fp), "186471204942302.4128");
  }
}

TEST(FixedPointTest, ParseErrors) {
  EXPECT_FALSE(FixedPoint<4>::Parse(""));

  EXPECT_FALSE(FixedPoint<4>::Parse("XXX"));
  EXPECT_FALSE(FixedPoint<4>::Parse("0x1234"));
  EXPECT_FALSE(FixedPoint<4>::Parse("0b10010"));
  EXPECT_FALSE(FixedPoint<4>::Parse("X1b"));
  EXPECT_FALSE(FixedPoint<4>::Parse("x1b"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1xb"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1q"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1 "));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.0 "));
  EXPECT_FALSE(FixedPoint<4>::Parse(" 1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1 .0"));
  EXPECT_FALSE(FixedPoint<4>::Parse("++1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("+.1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("-.1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("+-1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("--1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("+++1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("+-+1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("--+1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("++-1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("+--1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("---1"));

  EXPECT_FALSE(FixedPoint<4>::Parse("+"));
  EXPECT_FALSE(FixedPoint<4>::Parse("-"));
  EXPECT_FALSE(FixedPoint<4>::Parse("++"));
  EXPECT_FALSE(FixedPoint<4>::Parse("--"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1+1"));

  EXPECT_FALSE(FixedPoint<4>::Parse("1."));
  EXPECT_FALSE(FixedPoint<4>::Parse(".1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.+"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.-"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.-1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.+1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.a"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.1+"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.1-"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.1."));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.1.0"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.1.1"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.1.+"));
  EXPECT_FALSE(FixedPoint<4>::Parse("1.1.-"));

  // Large and invalid (overflow)
  EXPECT_FALSE(FixedPoint<4>::Parse("9999999999999999999"));
  EXPECT_FALSE(FixedPoint<4>::Parse("10000000000000000000.0"));
}

TEST(FixedPointTest, Subtraction) {
  {
    auto lhs_int = 0;
    auto rhs_int = 0;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(0)));
  }
  {
    auto lhs_int = 10;
    auto rhs_int = 0;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(10)));
  }
  {
    auto lhs_int = 10;
    auto rhs_int = 5;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(5)));
  }
  {
    auto lhs_int = 5;
    auto rhs_int = 5;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(0)));
  }
  {
    auto lhs_int = 10000;
    auto rhs_int = 0;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(10000)));
  }
  {
    auto lhs_int = 10000;
    auto rhs_int = 5000;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(5000)));
  }
  {
    auto lhs_int = 0;
    auto rhs_int = 10;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(-10)));
  }
  {
    auto lhs_int = 0;
    auto rhs_int = -1;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(1)));
  }
  {
    auto lhs_int = -1;
    auto rhs_int = 0;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(-1)));
  }
  {
    auto lhs_int = -1;
    auto rhs_int = -1;
    auto lhs = FixedPoint<4>(lhs_int);
    auto rhs = FixedPoint<4>(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_test_string(result), to_test_string(FixedPoint<4>(0)));
  }
}

template <typename FixedPoint>
void assertFixedPointComparisons(auto lhs, auto rhs) {
  auto lhs_fp = FixedPoint(lhs);
  auto rhs_fp = FixedPoint(rhs);

  EXPECT_EQ(lhs == rhs, lhs_fp == rhs_fp);
  EXPECT_EQ(lhs < rhs, lhs_fp < rhs_fp);
  EXPECT_EQ(lhs <= rhs, lhs_fp <= rhs_fp);
  EXPECT_EQ(lhs > rhs, lhs_fp > rhs_fp);
  EXPECT_EQ(lhs >= rhs, lhs_fp >= rhs_fp);
  EXPECT_EQ(lhs != rhs, lhs_fp != rhs_fp);
}

TEST(FixedPointTest, Comparisons) {
  assertFixedPointComparisons<FixedPoint<4>>(0, 0);
  assertFixedPointComparisons<FixedPoint<4>>(10, 0);
  assertFixedPointComparisons<FixedPoint<4>>(10, 5);
  assertFixedPointComparisons<FixedPoint<4>>(5, 5);
  assertFixedPointComparisons<FixedPoint<4>>(10000, 0);
  assertFixedPointComparisons<FixedPoint<4>>(10000, 5000);
  assertFixedPointComparisons<FixedPoint<4>>(0, 10);
  assertFixedPointComparisons<FixedPoint<4>>(0, -1);
  assertFixedPointComparisons<FixedPoint<4>>(-1, 0);
  assertFixedPointComparisons<FixedPoint<4>>(-1, -1);
}
