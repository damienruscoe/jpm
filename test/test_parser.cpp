#include <gtest/gtest.h>
#include "../src/parser.hpp"
#include <string_view>
#include <vector>

struct ValidParserTestData {
    std::string input;
    std::string description;
};

class ParserValidTest : public ::testing::TestWithParam<ValidParserTestData> {};

TEST_P(ParserValidTest, SucceedsOnValidInput) {
    const auto& param = GetParam();
    auto msg = parse_line(param.input);
    EXPECT_TRUE(msg.has_value()) << "Failed to parse valid input (" << param.description << "): " << param.input;
}

INSTANTIATE_TEST_SUITE_P(
    ValidInputs,
    ParserValidTest,
    ::testing::Values(
        ValidParserTestData{"101,N,A001,B,1000,3.2", "Standard message"},
        ValidParserTestData{" 108,N,A008,S,1000,8.0  ", "Whitespace trimming"},
        ValidParserTestData{"4294967295,N,MAX-ID,S,4294967295,9999.99", "Boundaries"}
    )
);

TEST(ParserValidTest, HandlesFunctionalSequence) {
    const std::vector<std::string> lines = {
        "101,N,A001,S,3000,6.8",
        "101,N,A002,S,1000,6.9",
        "101,N,A003,B,2000,6.7",
        "101,N,A004,B,1000,6.8 // crosses with order A001 with price 6.8",
        "101,A,A003,B,2000,6.9 // after amending A003 crosses with A001 and both fully filled",
        "102,N,A005,S,2000,10.2",
        "102,N,A006,B,2000,10.1",
        "102,N,A007,B,2000,10.1",
        "102,C,A006,B,2000,10.1 // A006 will be removed from order book"
    };

    for (const auto& line : lines) {
        EXPECT_TRUE(parse_line(line).has_value()) << "Failed to parse functional line: " << line;
    }
}

struct InvalidParserTestData {
    std::string input;
    std::string description;
};

class ParserInvalidTest : public ::testing::TestWithParam<InvalidParserTestData> {};

TEST_P(ParserInvalidTest, RejectsInput) {
    const auto& param = GetParam();
    auto msg = parse_line(param.input);
    EXPECT_FALSE(msg.has_value()) << "Accepted invalid input (" << param.description << "): " << param.input;
}

INSTANTIATE_TEST_SUITE_P(
    InvalidInputs,
    ParserInvalidTest,
    ::testing::Values(
        InvalidParserTestData{"101,N,A001,B,1000", "Missing columns"},
        InvalidParserTestData{"101,N,A001,B,1000,3.2,EXTRA", "Too many columns"},
        InvalidParserTestData{"101,N,12345678901,B,1000,3.2", "ID too long"},
        InvalidParserTestData{"101,N,A#001,B,1000,3.2", "Invalid char #"},
        InvalidParserTestData{"", "Empty line"},
        InvalidParserTestData{"0,N,A001,B,1000,3.2", "Ticker 0"},
        InvalidParserTestData{"ABC,N,A001,B,1000,3.2", "Non-numeric ticker"},
        InvalidParserTestData{"101A,N,A001,B,1000,3.2", "Ticker with letters"},
        InvalidParserTestData{"101,X,A001,B,1000,3.2", "Invalid request type"},
        InvalidParserTestData{"101,N,,B,1000,3.2", "Empty ID"},
        InvalidParserTestData{"101,N,A001,X,1000,3.2", "Invalid side X"},
        InvalidParserTestData{"101,N,A001,B,0,3.2", "Qty 0"},
        InvalidParserTestData{"101,N,A001,B,-10,3.2", "Negative Qty"},
        InvalidParserTestData{"101,N,A001,B,1000,ABC", "Non-numeric price"},
        InvalidParserTestData{"101,N,A001,B,1000,3.2.1", "Multiple dots price"}
    )
);
