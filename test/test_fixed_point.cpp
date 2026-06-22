#include "../src/FixedPoint.hpp"
#include <gtest/gtest.h>
#include <sstream>

TEST(FixedPointTest, ParseErrors) {
  // Test empty string
  EXPECT_THROW(FixedPointAI::Parse(""), std::invalid_argument);

  // Test fractional part missing/invalid
  EXPECT_THROW(FixedPointAI::Parse("1."), std::invalid_argument);
  EXPECT_THROW(FixedPointAI::Parse("1.a"), std::invalid_argument);
  EXPECT_THROW(FixedPointAI::Parse("+"), std::invalid_argument);
  EXPECT_THROW(FixedPointAI::Parse("-"), std::invalid_argument);

  // Test too many decimal places (the logic handles it, but let's ensure the
  // path is hit) The loop: for (size_t i = 4; i < digits; ++i) fractional /=
  // 10; We need more than 4 fractional digits.
  FixedPointAI fp = FixedPointAI::Parse("1.12345");
  EXPECT_EQ(fp.ToDouble(), 1.1234); // Should truncate
}

TEST(FixedPointTest, Operations) {
  FixedPointAI a(10000); // 1.0000
  FixedPointAI b(5000);  // 0.5000

  // Test GetRaw
  EXPECT_EQ(a.GetRaw(), 10000);

  // Test operator-
  FixedPointAI c = a - b;
  EXPECT_EQ(c.GetRaw(), 5000);

  // Test operator<<
  std::stringstream ss;
  ss << a;
  EXPECT_EQ(ss.str(), "1"); // ToDouble() returns 1.0 for 1.0000, ostream uses
                            // default double formatting
}
