#include "../src/FixedPointGeneric.hpp"
#include <gtest/gtest.h>
#include <sstream>

template <int N> auto to_string(const FixedPoint<N> &fp) {
  std::stringstream ss;
  ss << fp;
  return ss.str();
}

TEST(FixedPointTest, ParseErrors) {
  // Test empty string
  EXPECT_THROW(FixedPointGeneric::Parse(""), std::invalid_argument);

  // Test fractional part missing/invalid
  EXPECT_THROW(FixedPointGeneric::Parse("1."), std::invalid_argument);
  EXPECT_THROW(FixedPointGeneric::Parse("1.a"), std::invalid_argument);
  EXPECT_THROW(FixedPointGeneric::Parse("+"), std::invalid_argument);
  EXPECT_THROW(FixedPointGeneric::Parse("-"), std::invalid_argument);

  {
    auto fp = FixedPoint<0>::Parse("1.12345");
    EXPECT_EQ(to_string(fp), "1.0");
  }
  {
    auto fp = FixedPoint<1>::Parse("1.12345");
    EXPECT_EQ(to_string(fp), "1.1");
  }
  {
    auto fp = FixedPoint<2>::Parse("1.12345");
    EXPECT_EQ(to_string(fp), "1.12");
  }
  {
    auto fp = FixedPoint<3>::Parse("1.12345");
    EXPECT_EQ(to_string(fp), "1.123");
  }
  {
    auto fp = FixedPoint<4>::Parse("1.12345");
    EXPECT_EQ(to_string(fp), "1.1234");
  }
  {
    auto fp = FixedPoint<5>::Parse("1.12345");
    EXPECT_EQ(to_string(fp), "1.12345");
  }
  {
    auto fp = FixedPoint<6>::Parse("1.12345");
    EXPECT_EQ(to_string(fp), "1.123450");
  }
  {
    auto fp = FixedPoint<7>::Parse("1.12345");
    EXPECT_EQ(to_string(fp), "1.1234500");
  }
}

TEST(FixedPointTest, Operations) {
  FixedPointGeneric a(10000); // 1.0000
  FixedPointGeneric b(5000);  // 0.5000

  // Test operator<<
  std::stringstream ss;
  ss << a;
  EXPECT_EQ(ss.str(), "1.0000"); // ToDouble() returns 1.0 for 1.0000, ostream
                                 // uses default double formatting
}
