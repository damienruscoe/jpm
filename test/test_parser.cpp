#include <gtest/gtest.h>
#include "../src/parser.hpp"

struct ValidParserTestData {
    std::string input;
    std::string description;
};

class ParserValidTest : public ::testing::TestWithParam<ValidParserTestData> {};

TEST_P(ParserValidTest, ValidInput) {
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
        ValidParserTestData{"101,N,00101,B,007,3.2", "Leading zeros"},
        ValidParserTestData{"101,N,00101,B,009,3.2", "Leading zeros"},
        ValidParserTestData{"101,N,N,B,1000,3.2", "ID is RequestType N"},
        ValidParserTestData{"101,N,C,B,1000,3.2", "ID is RequestType C"},
        ValidParserTestData{"101,N,A,B,1000,3.2", "ID is RequestType A"},
        ValidParserTestData{"101,N,A001,B,1000,+0.0000", "Positive signed zero"},
        ValidParserTestData{"101,N,A001,B,1000,-0.0000", "Negative signed zero"},
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

