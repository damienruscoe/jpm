#include <gtest/gtest.h>
#include "../src/parser.hpp"

struct ValidParserTestData {
    std::string input;
    std::string description;
    Message expected;
};

void PrintTo(const ValidParserTestData& data, ::std::ostream* os) {
    *os << data.description << " (Input: \"" << data.input << "\")";
}

class ParserValidTest : public ::testing::TestWithParam<ValidParserTestData> {};

TEST_P(ParserValidTest, ValidInput) {
    const auto& param = GetParam();
    auto msg = parse_line(param.input);
    ASSERT_TRUE(msg.has_value()) << "Failed to parse valid input (" << param.description << "): " << param.input;
    
    EXPECT_EQ(msg->exchange_ticker, param.expected.exchange_ticker);
    EXPECT_EQ(msg->type, param.expected.type);
    EXPECT_EQ(msg->order_id, param.expected.order_id);
    EXPECT_EQ(msg->side, param.expected.side);
    EXPECT_EQ(msg->quantity, param.expected.quantity);
    EXPECT_DOUBLE_EQ(msg->price.ToDouble(), param.expected.price.ToDouble());
}

INSTANTIATE_TEST_SUITE_P(
    ValidInputs,
    ParserValidTest,
    ::testing::Values(
        ValidParserTestData{"101,N,A001,B,1000,3.2", "Standard message", {101, RequestType::New, "A001", Side::Buy, 1000, FixedPointAI(32000)}},
        ValidParserTestData{"108,N,A008,S,1000,8.0  ", "Whitespace trimming", {108, RequestType::New, "A008", Side::Sell, 1000, FixedPointAI(80000)}},
        ValidParserTestData{"108,N,A008,S,1000,8.0 // , Comment with comma", "Comment with comma", {108, RequestType::New, "A008", Side::Sell, 1000, FixedPointAI(80000)}},
        ValidParserTestData{"4294967295,N,MAX-ID,S,4294967295,9999.99", "Boundaries (Max uint32)", {4294967295, RequestType::New, "MAX-ID", Side::Sell, 4294967295, FixedPointAI(99999900)}},
        ValidParserTestData{"101,N,0000000000,B,1000,3.2", "Max ID length (10)", {101, RequestType::New, "0000000000", Side::Buy, 1000, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,ABCDEFGHIJ,B,1000,3.2", "Max ID length (10)", {101, RequestType::New, "ABCDEFGHIJ", Side::Buy, 1000, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,1234567890,B,1000,3.2", "Max ID length (10)", {101, RequestType::New, "1234567890", Side::Buy, 1000, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,---,B,1000,3.2", "Only hyphens ID", {101, RequestType::New, "---", Side::Buy, 1000, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,A-B-C-1,B,1,1.1111", "Hyphens in ID", {101, RequestType::New, "A-B-C-1", Side::Buy, 1, FixedPointAI(11111)}},
        ValidParserTestData{"1,A,A-1,S,1,0.0001", "Min values", {1, RequestType::Amend, "A-1", Side::Sell, 1, FixedPointAI(1)}},
        ValidParserTestData{"101,N,00101,B,007,3.2", "Leading zeros ID/Qty", {101, RequestType::New, "00101", Side::Buy, 7, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,00101,B,010,3.2", "Leading zeros ID/Qty", {101, RequestType::New, "00101", Side::Buy, 10, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,N,B,1000,3.2", "ID is RequestType N", {101, RequestType::New, "N", Side::Buy, 1000, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,C,B,1000,3.2", "ID is RequestType C", {101, RequestType::New, "C", Side::Buy, 1000, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,A,B,1000,3.2", "ID is RequestType A", {101, RequestType::New, "A", Side::Buy, 1000, FixedPointAI(32000)}},
        ValidParserTestData{"101,N,A001,B,1000,+0.0000", "Positive signed zero", {101, RequestType::New, "A001", Side::Buy, 1000, FixedPointAI(0)}},
        ValidParserTestData{"101,N,A001,B,1000,-0.0000", "Negative signed zero", {101, RequestType::New, "A001", Side::Buy, 1000, FixedPointAI(0)}},
        ValidParserTestData{"0000000000101,N,A001,B,1000,0.0000", "Zero padded ticker ID", {101, RequestType::New, "A001", Side::Buy, 1000, FixedPointAI(0)}},
        ValidParserTestData{"101,N,A001,B,00000000001000,0.0000", "Zero padded quantity", {101, RequestType::New, "A001", Side::Buy, 1000, FixedPointAI(0)}},
        // Provided example
        ValidParserTestData{"101,N,A001,S,3000,6.8", "Example Seq 1", {101, RequestType::New, "A001", Side::Sell, 3000, FixedPointAI(68000)}},
        ValidParserTestData{"101,N,A002,S,1000,6.9", "Example Seq 2", {101, RequestType::New, "A002", Side::Sell, 1000, FixedPointAI(69000)}},
        ValidParserTestData{"101,N,A003,B,2000,6.7", "Example Seq 3", {101, RequestType::New, "A003", Side::Buy, 2000, FixedPointAI(67000)}},
        ValidParserTestData{"101,N,A004,B,1000,6.8 // comment", "Example Seq 4 (comment)", {101, RequestType::New, "A004", Side::Buy, 1000, FixedPointAI(68000)}},
        ValidParserTestData{"101,A,A003,B,2000,6.9 // comment", "Example Seq 5 (comment)", {101, RequestType::Amend, "A003", Side::Buy, 2000, FixedPointAI(69000)}},
        ValidParserTestData{"102,N,A005,S,2000,10.2", "Example Seq 6", {102, RequestType::New, "A005", Side::Sell, 2000, FixedPointAI(102000)}},
        ValidParserTestData{"102,N,A006,B,2000,10.1", "Example Seq 7", {102, RequestType::New, "A006", Side::Buy, 2000, FixedPointAI(101000)}},
        ValidParserTestData{"102,N,A007,B,2000,10.1", "Example Seq 8", {102, RequestType::New, "A007", Side::Buy, 2000, FixedPointAI(101000)}},
        ValidParserTestData{"102,C,A006,B,2000,10.1 // comment", "Example Seq 9 (comment)", {102, RequestType::Cancel, "A006", Side::Buy, 2000, FixedPointAI(101000)}}
    )
);

