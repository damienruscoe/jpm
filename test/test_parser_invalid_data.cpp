#include "../src/parser.hpp"
#include <gtest/gtest.h>

struct InvalidParserTestData {
  std::string input;
  std::string description;
};

void PrintTo(const InvalidParserTestData &data, ::std::ostream *os) {
  *os << data.description << " (Input: \"" << data.input << "\")";
}

class ParserInvalidTest
    : public ::testing::TestWithParam<InvalidParserTestData> {};

TEST_P(ParserInvalidTest, InvalidInput) {
  const auto &param = GetParam();
  auto msg = parse_line(param.input);
  EXPECT_FALSE(msg.has_value()) << "Accepted invalid input ("
                                << param.description << "): " << param.input;
}

INSTANTIATE_TEST_SUITE_P(
    InvalidInputs, ParserInvalidTest,
    ::testing::Values(
        // Columns / Format
        InvalidParserTestData{"", "Empty line"},
        InvalidParserTestData{"   \t  ", "Whitespace only"},
        InvalidParserTestData{"#comment only",
                              "Comment only (valid but ignored)"},
        InvalidParserTestData{"// comment only",
                              "Comment only (valid but ignored)"},
        InvalidParserTestData{"101,N,A001,B,1000", "Missing columns"},
        InvalidParserTestData{"101,N,A001,B,1000,3.2,EXTRA",
                              "Too many columns"},
        InvalidParserTestData{",,,,,,", "Empty fields"},

        // Ticker
        InvalidParserTestData{" 101,N,A001,S,3000,6.8", "Space Before Ticker"},
        InvalidParserTestData{"101 ,N,A001,S,3000,6.8", "Space After Ticker"},
        InvalidParserTestData{" 101 ,N,A001,B,1000,3.2",
                              "Multiple Spaces in ticker field"},
        InvalidParserTestData{",N,A001,S,3000,6.8", "Missing Ticker"},
        InvalidParserTestData{"0,N,A001,B,1000,3.2", "Ticker 0"},
        InvalidParserTestData{"-5,N,A001,B,1000,3.2", "Negative ticker"},
        InvalidParserTestData{"ABC,N,A001,B,1000,3.2", "Non-numeric ticker"},
        InvalidParserTestData{"101A,N,A001,B,1000,3.2", "Trailing ticker junk"},
        InvalidParserTestData{"101 101,N,A001,B,1000,3.2",
                              "Ticker with spaces"},
        InvalidParserTestData{"4294967296,N,A001,B,1000,3.2",
                              "Ticker Overflow"},
        InvalidParserTestData{"-2147483648,N,A001,B,1000,3.2",
                              "Ticker Underflow"},

        // Type
        InvalidParserTestData{"101, N,A001,S,3000,6.8", "Space Before Type"},
        InvalidParserTestData{"101,N ,A001,S,3000,6.8", "Space After Type"},
        InvalidParserTestData{"101, N ,A001,B,1000,3.2",
                              "Multiple Space In Type field"},
        InvalidParserTestData{"101,,A001,S,3000,6.8", "Missing Type"},
        InvalidParserTestData{"101,X,A001,B,1000,3.2",
                              "Invalid request type X"},
        InvalidParserTestData{"101,n,A001,B,1000,3.2", "Lowercase type"},
        InvalidParserTestData{"101,NN,A001,S,3000,6.8",
                              "Valid type multiple letters"},
        InvalidParserTestData{"101,CC,A001,S,3000,6.8",
                              "Valid type multiple letters"},
        InvalidParserTestData{"101,AA,A001,S,3000,6.8",
                              "Valid type multiple letters"},
        InvalidParserTestData{"101,NCA,A001,S,3000,6.8",
                              "Valid type multiple letters"},

        // Order ID
        InvalidParserTestData{"101,N, A001,S,3000,6.8", "Space Before OrderID"},
        InvalidParserTestData{"101,N,A001 ,S,3000,6.8", "Space After OrderID"},
        InvalidParserTestData{"101,N, A001 ,B,1000,3.2",
                              "Multiple Spaces in ID field"},
        InvalidParserTestData{"101,N,,S,3000,6.8", "Missing ID"},
        InvalidParserTestData{"101,N,,B,1000,3.2", "Empty ID"},
        InvalidParserTestData{"101,N,TOO-LONG-ID-12345,B,1000,3.2",
                              "ID too long"},
        InvalidParserTestData{"101,N,A#001,B,1000,3.2", "Invalid char #"},
        InvalidParserTestData{"101,N,A_001,B,1000,3.2", "Invalid char _"},
        InvalidParserTestData{"101,N,A\t001,B,1000,3.2", "Tab in ID"},
        InvalidParserTestData{"101,N,A\v001,B,1000,3.2", "Vertical tab in ID"},
        InvalidParserTestData{"101,N,A\f001,B,1000,3.2", "Form feed in ID"},
        InvalidParserTestData{"101,N,☃,B,1000,3.2", "Unicode in ID"},

        // Side
        InvalidParserTestData{"101,N,A001, S,3000,6.8", "Space Before Side"},
        InvalidParserTestData{"101,N,A001,S ,3000,6.8", "Space After Side"},
        InvalidParserTestData{"101,N,A001, B ,1000,3.2",
                              "Multiple Spaces in Side field"},
        InvalidParserTestData{"101,N,A001,,3000,6.8", "Missing Side"},
        InvalidParserTestData{"101,N,A001,X,1000,3.2", "Invalid side X"},
        InvalidParserTestData{"101,N,A001,b,1000,3.2", "Lowercase side"},
        InvalidParserTestData{"101,N,A001,SS,3000,6.8",
                              "Valid Side letters but too long"},
        InvalidParserTestData{"101,N,A001,BB,3000,6.8",
                              "Valid Side letters but too long"},
        InvalidParserTestData{"101,N,A001,BS,3000,6.8",
                              "Valid Side letters but too long"},
        InvalidParserTestData{"101,N,A001,SB,3000,6.8",
                              "Valid Side letters but too long"},

        // Quantity
        InvalidParserTestData{"102,A,A001,B,0,10.2", "Zero Quantity"},
        InvalidParserTestData{"102,A,A001,B,-300,10.2", "Negative Quantity"},
        InvalidParserTestData{"102,A,A001,B,999999999999999999,10.2",
                              "OOB Quantity"},
        InvalidParserTestData{"102,A,A001,B,-999999999999999999,10.2",
                              "Negative OOB Quantity"},
        InvalidParserTestData{"101,N,A001,S, 3000,6.8",
                              "Space Before Quantity"},
        InvalidParserTestData{"101,N,A001,S,3000 ,6.8", "Space After Quantity"},
        InvalidParserTestData{"101,N,A001,S,,6.8", "Missing Qty"},
        InvalidParserTestData{"101,N,A001,B,0,3.2", "Qty 0"},
        InvalidParserTestData{"101,N,A001,B,-10,3.2", "Negative Qty"},
        InvalidParserTestData{"101,N,A001,B,1000.5,3.2", "Fractional Qty"},
        InvalidParserTestData{"101,N,A001,B,ABC,3.2", "Non-numeric Qty"},
        InvalidParserTestData{"101,N,A001,B,4294967296,3.2", "Qty Overflow"},
        InvalidParserTestData{"101,N,A001,B, 1000 ,3.2",
                              "Multiple Spaces in Qty field"},

        // Price
        InvalidParserTestData{"102,A,A001,B,3000,0", "Zero Price"},
        InvalidParserTestData{"102,A,A001,B,3000,-0", "Signed Zero Price"},
        InvalidParserTestData{"102,A,A001,B,3000,+0", "Signed Zero Price"},
        InvalidParserTestData{"102,A,A001,B,3000,-20.1", "Negative Price"},
        InvalidParserTestData{"102,A,A001,B,3000,9999999999999999999",
                              "OOB Price"},
        InvalidParserTestData{"102,A,A001,B,3000,-9999999999999999999",
                              "Negative OOB Price"},
        InvalidParserTestData{"102,A,A001,B,3000,10000000000000000000.0",
                              "OOB Price"},
        InvalidParserTestData{"102,A,A001,B,3000,-10000000000000000000.0",
                              "Negative OOB Price"},
        InvalidParserTestData{"102,A,A001,B,3000,0.0000000000000000001",
                              "OOB Decimal Price"},
        InvalidParserTestData{"102,A,A001,B,3000,-0.0000000000000000001",
                              "Negative OOB Decimal Price"},
        InvalidParserTestData{"101,N,A001,S,3000, 6.8", "Space Before Price"},
        InvalidParserTestData{"101,N,A001,S,3000,6.8 ,",
                              "Space After Price and another delimiter"},
        InvalidParserTestData{"101,N,A001,S,3000,6.8,",
                              "Delimiter after Price"},
        InvalidParserTestData{"101,N,A001,S,3000,", "Missing Price"},
        InvalidParserTestData{"101,N,A001,B,1000,ABC", "Non-numeric price"},
        InvalidParserTestData{"101,N,A001,B,1000,3.2.1", "Multiple dots"},
        InvalidParserTestData{"101,N,A001,B,1000,1e2", "Scientific notation"},
        InvalidParserTestData{"101,N,A001,B,1000,.", "Only dot"},
        InvalidParserTestData{"101,N,A001,B,1000,3.", "Trailing dot"},
        InvalidParserTestData{"101,N,A001,B,1000, 3.2 ",
                              "Multiple Spaces in Price field"},
        InvalidParserTestData{"101,N,A001,B,1000,- 3.2",
                              "Space between sign and price"},
        InvalidParserTestData{"101,N,A001,B,1000,+ 3.2",
                              "Space between sign and price"},

        // Delimiter and Column Count
        InvalidParserTestData{"101|N|A001|B|1000|3.2", "Pipe delimiter"},
        InvalidParserTestData{"101:N:A001:B:1000:3.2", "Colon delimiter"},
        InvalidParserTestData{"101;N;A001;B;1000;3.2", "Semicolon delimiter"},
        InvalidParserTestData{"101`N,A001,B,1000,3.2", "Malformed delimiter 1"},
        InvalidParserTestData{"101,N`A001,B,1000,3.2", "Malformed delimiter 2"},
        InvalidParserTestData{"101,N,A001`B,1000,3.2", "Malformed delimiter 3"},
        InvalidParserTestData{"101,N,A001,B`1000,3.2", "Malformed delimiter 4"},
        InvalidParserTestData{"101,N,A001,B,1000`3.2", "Malformed delimiter 5"},
        InvalidParserTestData{"101,N,A001,B,1000,3.2,",
                              "Extra delimiter at end"},
        InvalidParserTestData{",101,N,A001,B,1000,3.2",
                              "Extra delimiter at start"},

        // Swapped Fields
        InvalidParserTestData{"N,101,A001,B,1000,3.2", "Ticker/Type swapped"},
        InvalidParserTestData{"101,A001,N,B,1000,3.2", "Type/ID swapped"},
        InvalidParserTestData{"101,N,A001,1000,B,3.2", "Side/Qty swapped"},
        InvalidParserTestData{"101,N,A001,B,3.2,1000", "Qty/Price swapped"}));
