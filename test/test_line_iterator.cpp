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

TEST(LineViewTest, HandlesEmpty) {
  LineView lines(nullptr, 0);
  EXPECT_EQ(lines.begin(), lines.end());

  const std::string data = "";
  LineView empty_lines(data.data(), data.size());
  EXPECT_EQ(empty_lines.begin(), empty_lines.end());
}

TEST(LineViewTest, HandlesOnlyNewlines) {
  const std::string data = "\n\n\n";
  LineView lines(data.data(), data.size());

  int count = 0;
  for (auto line : lines) {
    EXPECT_EQ(line, "");
    count++;
  }
  EXPECT_EQ(count, 3);
}

TEST(LineViewTest, HandlesTrailingNewline) {
  const std::string data = "line1\n";
  LineView lines(data.data(), data.size());

  std::vector<std::string_view> results;
  for (auto line : lines) {
    results.push_back(line);
  }
  ASSERT_EQ(results.size(), 1);
  EXPECT_EQ(results[0], "line1");
}

TEST(LineViewTest, IteratorComparison) {
  const std::string data = "line1\nline2";
  LineView lines(data.data(), data.size());

  auto it = lines.begin();
  auto it2 = it;
  EXPECT_EQ(it, it2);
  ++it;
  EXPECT_NE(it, it2);

  // Test -> operator
  EXPECT_EQ(it->size(), 5);
}
