#include <gtest/gtest.h>
#include "../src/parser.hpp"
#include <string_view>
#include <vector>

// --- Valid Test Cases ---
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
        ValidParserTestData{"4294967295,N,MAX-ID,S,4294967295,9999.99", "Boundaries (Max uint32)"},
        ValidParserTestData{"101,N,A123456789,B,1,0.0001", "Max ID length (10)"},
        ValidParserTestData{"101,N,A-B-C-1,B,1,1.1111", "Hyphens in ID"},
        ValidParserTestData{"1,A,A-1,S,1,0.0001", "Min values"},
        // Functional sequences (from given_example.csv)
        ValidParserTestData{"101,N,A001,S,3000,6.8", "Example Seq 1"},
        ValidParserTestData{"101,N,A002,S,1000,6.9", "Example Seq 2"},
        ValidParserTestData{"101,N,A003,B,2000,6.7", "Example Seq 3"},
        ValidParserTestData{"101,N,A004,B,1000,6.8 // comment", "Example Seq 4 (comment)"},
        ValidParserTestData{"101,A,A003,B,2000,6.9 // comment", "Example Seq 5 (comment)"},
        ValidParserTestData{"102,N,A005,S,2000,10.2", "Example Seq 6"},
        ValidParserTestData{"102,N,A006,B,2000,10.1", "Example Seq 7"},
        ValidParserTestData{"102,N,A007,B,2000,10.1", "Example Seq 8"},
        ValidParserTestData{"102,C,A006,B,2000,10.1 // comment", "Example Seq 9 (comment)"}
    )
);

// --- Invalid Test Cases ---
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
        // Columns / Format
        InvalidParserTestData{"", "Empty line"},
        InvalidParserTestData{"#comment only", "Comment only (valid but ignored)"},
        InvalidParserTestData{"101,N,A001,B,1000", "Missing columns"},
        InvalidParserTestData{"101,N,A001,B,1000,3.2,EXTRA", "Too many columns"},
        InvalidParserTestData{",,,,,,", "Empty fields"},
        
        // Ticker
        InvalidParserTestData{"0,N,A001,B,1000,3.2", "Ticker 0"},
        InvalidParserTestData{"-1,N,A001,B,1000,3.2", "Negative ticker"},
        InvalidParserTestData{"ABC,N,A001,B,1000,3.2", "Non-numeric ticker"},
        InvalidParserTestData{"101A,N,A001,B,1000,3.2", "Trailing ticker junk"},
        InvalidParserTestData{"101 101,N,A001,B,1000,3.2", "Ticker with spaces"},
        InvalidParserTestData{"4294967296,N,A001,B,1000,3.2", "Ticker Overflow"},
        
        // Type
        InvalidParserTestData{"101,X,A001,B,1000,3.2", "Invalid request type X"},
        InvalidParserTestData{"101,n,A001,B,1000,3.2", "Lowercase type"},
        
        // Order ID
        InvalidParserTestData{"101,N,,B,1000,3.2", "Empty ID"},
        InvalidParserTestData{"101,N,TOO-LONG-ID-12345,B,1000,3.2", "ID too long"},
        InvalidParserTestData{"101,N,A#001,B,1000,3.2", "Invalid char #"},
        InvalidParserTestData{"101,N,A_001,B,1000,3.2", "Invalid char _"},
        
        // Side
        InvalidParserTestData{"101,N,A001,X,1000,3.2", "Invalid side X"},
        InvalidParserTestData{"101,N,A001,b,1000,3.2", "Lowercase side"},
        
        // Quantity
        InvalidParserTestData{"101,N,A001,B,0,3.2", "Qty 0"},
        InvalidParserTestData{"101,N,A001,B,-10,3.2", "Negative Qty"},
        InvalidParserTestData{"101,N,A001,B,1000.5,3.2", "Fractional Qty"},
        InvalidParserTestData{"101,N,A001,B,ABC,3.2", "Non-numeric Qty"},
        InvalidParserTestData{"101,N,A001,B,4294967296,3.2", "Qty Overflow"},
        
        // Price
        InvalidParserTestData{"101,N,A001,B,1000,ABC", "Non-numeric price"},
        InvalidParserTestData{"101,N,A001,B,1000,3.2.1", "Multiple dots"},
        InvalidParserTestData{"101,N,A001,B,1000,1e2", "Scientific notation"},
        InvalidParserTestData{"101,N,A001,B,1000,.", "Only dot"},
        InvalidParserTestData{"101,N,A001,B,1000,3.", "Trailing dot"}
    )
);
