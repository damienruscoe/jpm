#include <gtest/gtest.h>
#include "../src/line_view.hpp"
#include <string_view>
#include <vector>

TEST(LineViewTest, HandlesLineEndings) {
    const std::string data = "101,N,A001,B,1000,1.0\n102,N,A002,S,1000,2.0\r\n103,N,A003,B,1000,3.0\n\nwibble";
    LineView lines(data.data(), data.size());
    
    std::vector<std::string_view> results;
    for (auto line : lines) {
        results.push_back(line);
    }
    
    ASSERT_EQ(results.size(), 5);
    EXPECT_EQ(results[0], "101,N,A001,B,1000,1.0");
    EXPECT_EQ(results[1], "102,N,A002,S,1000,2.0");
    EXPECT_EQ(results[2], "103,N,A003,B,1000,3.0");
    EXPECT_EQ(results[3], "");
    EXPECT_EQ(results[4], "wibble");
}
