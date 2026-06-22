#include "../src/line_view.hpp"
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

TEST(LineViewTest, HandlesLineEndings) {
  const std::string data =
      "First line\n2nd Line \r\n  THE THIRD LINE  \n\nwibble";
  LineView lines(data.data(), data.size());

  std::vector<std::string_view> results;
  for (auto line : lines) {
    results.push_back(line);
  }

  ASSERT_EQ(results.size(), 5);
  EXPECT_EQ(results[0], "First line");
  EXPECT_EQ(results[1], "2nd Line ");
  EXPECT_EQ(results[2], "  THE THIRD LINE  ");
  EXPECT_EQ(results[3], "");
  EXPECT_EQ(results[4], "wibble");
}
