#include "../src/fixed_point.hpp"
#include <gtest/gtest.h>
#include <string>

// 1. Type-parameterized test setup
template <typename T> class FixedPointTest : public ::testing::Test {
protected:
  using FPType = T;
};

using FixedPointTypes = ::testing::Types<FixedPoint<4>>;
TYPED_TEST_SUITE(FixedPointTest, FixedPointTypes);

// 2. Data-parameterized test structures
struct ParseTestData {
  std::string input;
  int64_t expected_raw;
};

class ParseTest
    : public ::testing::TestWithParam<std::pair<std::string, ParseTestData>> {};

static auto to_string(const auto &fp) {
  std::stringstream ss;
  ss << fp;
  return ss.str();
}

TYPED_TEST(FixedPointTest, DefaultInitialized) {
  auto fp = TypeParam();
  EXPECT_EQ(to_string(fp), "0.0000");
}

TYPED_TEST(FixedPointTest, ValueInitialized) {
  {
    auto fp = TypeParam(0);
    EXPECT_EQ(to_string(fp), "0.0000");
  }
  {
    auto fp = TypeParam(1);
    EXPECT_EQ(to_string(fp), "1.0000");
  }
  {
    auto fp = TypeParam(2);
    EXPECT_EQ(to_string(fp), "2.0000");
  }
  {
    auto fp = TypeParam(10000);
    EXPECT_EQ(to_string(fp), "10000.0000");
  }
  {
    auto fp = TypeParam(20000);
    EXPECT_EQ(to_string(fp), "20000.0000");
  }
  {
    auto fp = TypeParam(-1);
    EXPECT_EQ(to_string(fp), "-1.0000");
  }
  {
    auto fp = TypeParam(-10);
    EXPECT_EQ(to_string(fp), "-10.0000");
  }
}

TYPED_TEST(FixedPointTest, Parsing) {
  EXPECT_THROW(TypeParam::Parse("1."), std::invalid_argument);
  EXPECT_THROW(TypeParam::Parse(".1"), std::invalid_argument);
  EXPECT_THROW(TypeParam::Parse("1.a"), std::invalid_argument);
  EXPECT_THROW(TypeParam::Parse("+"), std::invalid_argument);
  EXPECT_THROW(TypeParam::Parse("-"), std::invalid_argument);
  {
    auto fp = TypeParam::Parse("0");
    EXPECT_EQ(to_string(fp), "0.0000");
  }
  {
    auto fp = TypeParam::Parse("1");
    EXPECT_EQ(to_string(fp), "1.0000");
  }
  {
    auto fp = TypeParam::Parse("-1");
    EXPECT_EQ(to_string(fp), "-1.0000");
  }
  {
    auto fp = TypeParam::Parse("1.2345");
    EXPECT_EQ(to_string(fp), "1.2345");
  }
  {
    auto fp = TypeParam::Parse("00001");
    EXPECT_EQ(to_string(fp), "1.0000");
  }
  {
    auto fp = TypeParam::Parse("0.0001");
    EXPECT_EQ(to_string(fp), "0.0001");
  }
}

TYPED_TEST(FixedPointTest, Subtraction) {
  {
    auto lhs_int = 0;
    auto rhs_int = 0;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(0)));
  }
  {
    auto lhs_int = 10;
    auto rhs_int = 0;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(10)));
  }
  {
    auto lhs_int = 10;
    auto rhs_int = 5;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(5)));
  }
  {
    auto lhs_int = 5;
    auto rhs_int = 5;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(0)));
  }
  {
    auto lhs_int = 10000;
    auto rhs_int = 0;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(10000)));
  }
  {
    auto lhs_int = 10000;
    auto rhs_int = 5000;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(5000)));
  }
  {
    auto lhs_int = 0;
    auto rhs_int = 10;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(-10)));
  }
  {
    auto lhs_int = 0;
    auto rhs_int = -1;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(1)));
  }
  {
    auto lhs_int = -1;
    auto rhs_int = 0;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(-1)));
  }
  {
    auto lhs_int = -1;
    auto rhs_int = -1;
    auto lhs = TypeParam(lhs_int);
    auto rhs = TypeParam(rhs_int);
    auto result = lhs - rhs;
    EXPECT_EQ(to_string(result), to_string(TypeParam(0)));
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

TYPED_TEST(FixedPointTest, Comparisons) {
  assertFixedPointComparisons<TypeParam>(0, 0);
  assertFixedPointComparisons<TypeParam>(10, 0);
  assertFixedPointComparisons<TypeParam>(10, 5);
  assertFixedPointComparisons<TypeParam>(5, 5);
  assertFixedPointComparisons<TypeParam>(10000, 0);
  assertFixedPointComparisons<TypeParam>(10000, 5000);
  assertFixedPointComparisons<TypeParam>(0, 10);
  assertFixedPointComparisons<TypeParam>(0, -1);
  assertFixedPointComparisons<TypeParam>(-1, 0);
  assertFixedPointComparisons<TypeParam>(-1, -1);
}
