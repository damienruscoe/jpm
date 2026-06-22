#include "../src/FixedPointGeneric.hpp"
#include "../src/order_book.hpp"
#include "../src/parser.hpp"
#include <gtest/gtest.h>
#include <ranges>

struct Level {
  using OrderID = std::string;
  using Price = FixedPointGeneric;
  using Quantity = uint32_t;

  bool operator<(const Level &other) const {
    if (price != other.price)
      return price < other.price;
    return quantity < other.quantity;
  }
  bool operator==(const Level &other) const {
    return price == other.price && quantity == other.quantity;
  }

  Price price{};
  Quantity quantity{};
};

class OrderBookTest : public testing::Test {
protected:
  OrderBookTest() {}

  // ~OrderBookTest() override = default;

  using Book = OrderBook<Level>;
  Book book;
};

TEST_F(OrderBookTest, IsEmpty) {
  {
    auto top_of_book = book.getTopAsk(10);
    ASSERT_EQ(top_of_book.size(), 0);
    ASSERT_EQ(book.getBestBid(), std::nullopt);
    ASSERT_EQ(book.getBestAsk(), std::nullopt);
  }
  {
    auto top_of_book = book.getTopBid(10);
    ASSERT_EQ(top_of_book.size(), 0);
    ASSERT_EQ(book.getBestBid(), std::nullopt);
    ASSERT_EQ(book.getBestAsk(), std::nullopt);
  }
}

side_t convertSide(Side side) {
  return side == Side::Buy ? side_t::BID : side_t::ASK;
}

template <typename OrderBook>
bool processOrder(OrderBook &book, std::string_view dsl_add_order) {
  auto msg = parse_line(dsl_add_order);
  EXPECT_NE(msg, std::nullopt); // Test data provided the string. This is a test
                                // error on failure

  switch (msg->type) {
  case RequestType::New:
    return book.newOrder(msg->order_id, convertSide(msg->side), msg->price,
                         msg->quantity);
  case RequestType::Cancel:
    return book.cancel(msg->order_id, convertSide(msg->side));
  case RequestType::Amend:
    return book.amend(msg->order_id, convertSide(msg->side), msg->price,
                      msg->quantity);
  }
}

template <typename OrderBook>
void processOrderSuccess(OrderBook &book, std::string_view dsl_add_order) {
  bool success = processOrder(book, dsl_add_order);
  ASSERT_TRUE(success);
}

template <typename OrderBook>
void processOrderFailure(OrderBook &book, std::string_view dsl_add_order) {
  bool success = processOrder(book, dsl_add_order);
  ASSERT_FALSE(success);
}

template <typename Price>
void assertTopOfBook(const auto &top_of_book, const auto &expected) {
  ASSERT_EQ(top_of_book.size(), expected.size());
  size_t index = 0;
  for (const auto &[price, quantity] : expected) {
    ASSERT_EQ(top_of_book[index].price, Price::Parse(price));
    ASSERT_EQ(top_of_book[index].quantity, quantity);
    ++index;
  }
}

using ExpectedTopOfBook = std::vector<std::pair<std::string, uint64_t>>;

template <typename OrderBook>
void assertBookTopPriceLevels(const OrderBook &book,
                              const ExpectedTopOfBook &expected1,
                              const ExpectedTopOfBook &expected2) {
  auto reversed = ExpectedTopOfBook(expected1.rbegin(), expected1.rend());
  assertTopOfBook<typename OrderBook::Price>(book.getTopAsk(10), reversed);
  assertTopOfBook<typename OrderBook::Price>(book.getTopBid(10), expected2);
}

TEST_F(OrderBookTest, GivenExampleMarket1) {
  processOrderSuccess(book, "101,N,A001,S,3000,6.8");
  assertBookTopPriceLevels(book, {{"6.8", 3000}}, {});

  processOrderSuccess(book, "101,N,A002,S,1000,6.9");
  assertBookTopPriceLevels(book, {{"6.9", 1000}, {"6.8", 3000}}, {});

  processOrderSuccess(book, "101,N,A003,B,2000,6.7");
  assertBookTopPriceLevels(book, {{"6.9", 1000}, {"6.8", 3000}},
                           {{"6.7", 2000}});

  // crosses with order A001 with price 6.8
  processOrderSuccess(book, "101,N,A004,B,1000,6.8");
  assertBookTopPriceLevels(book, {{"6.9", 1000}, {"6.8", 2000}},
                           {{"6.7", 2000}});

  // after amending A003 crosses with A001 and both fully filled
  processOrderSuccess(book, "101,A,A003,B,2000,6.9");
  assertBookTopPriceLevels(book, {{"6.9", 1000}}, {});
}

TEST_F(OrderBookTest, GivenExampleMarket2) {
  processOrderSuccess(book, "102,N,A005,S,2000,10.2");
  assertBookTopPriceLevels(book, {{"10.2", 2000}}, {});

  processOrderSuccess(book, "102,N,A006,B,2000,10.1");
  assertBookTopPriceLevels(book, {{"10.2", 2000}}, {{"10.1", 2000}});

  processOrderSuccess(book, "102,N,A007,B,2000,10.1");
  assertBookTopPriceLevels(book, {{"10.2", 2000}}, {{"10.1", 4000}});

  // A006 will be removed from order book
  processOrderSuccess(book, "102,C,A006,B,2000,10.1");
  assertBookTopPriceLevels(book, {{"10.2", 2000}}, {{"10.1", 2000}});
}

TEST_F(OrderBookTest, GetBest) {
  processOrderSuccess(book, "102,N,A001,S,4000,13.0");
  processOrderSuccess(book, "102,N,A002,S,3000,12.0");
  processOrderSuccess(book, "102,N,A003,B,2000,11.0");
  processOrderSuccess(book, "102,N,A004,B,1000,10.0");

  ASSERT_EQ(book.getBestAsk()->price, Book::Price::Parse("12.0"));
  ASSERT_EQ(book.getBestAsk()->quantity, 3000);

  ASSERT_EQ(book.getBestBid()->price, Book::Price::Parse("11.0"));
  ASSERT_EQ(book.getBestBid()->quantity, 2000);
}

TEST_F(OrderBookTest, TestPriceTimePriority_TimePriority) {
  processOrderSuccess(book, "102,N,A001,S,1000,10.0");
  processOrderSuccess(book, "102,N,A002,S,1000,10.0");
  // A001 should be filled first (time priority)
  processOrderSuccess(book, "102,N,A003,B,1000,10.0");

  assertBookTopPriceLevels(book, {{"10.0", 1000}}, {});

  // Check A001 has been removed from the book
  processOrderFailure(book, "102,C,A001,S,1000,10.0");
  processOrderSuccess(book, "102,C,A002,S,1000,10.0");
}

TEST_F(OrderBookTest, TestPriceTimePriority_PricePriority) {
  processOrderSuccess(book, "102,N,A001,S,1000,11.0");
  processOrderSuccess(book, "102,N,A002,S,1000,10.0");
  // A002 should be filled first (time priority)
  processOrderSuccess(book, "102,N,A003,B,1000,10.0");

  assertBookTopPriceLevels(book, {{"11.0", 1000}}, {});

  // Check A002 has been removed from the book
  processOrderFailure(book, "102,C,A002,S,1000,10.0");
  processOrderSuccess(book, "102,C,A001,S,1000,11.0");
}

TEST_F(OrderBookTest, TestFillOrderSB) {
  processOrderSuccess(book, "102,N,A001,S,5000,10.2");
  processOrderSuccess(book, "102,N,A002,B,2000,10.2");

  assertBookTopPriceLevels(book, {{"10.2", 3000}}, {});
}

TEST_F(OrderBookTest, TestFillOrderBS) {
  processOrderSuccess(book, "102,N,A002,B,2000,10.2");
  processOrderSuccess(book, "102,N,A001,S,5000,10.2");

  assertBookTopPriceLevels(book, {{"10.2", 3000}}, {});
}

TEST_F(OrderBookTest, TestFillOrderWithRemaining) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  // Fully fill A001
  processOrderSuccess(book, "102,N,A002,B,5000,10.2");

  assertBookTopPriceLevels(book, {}, {{"10.2", 3000}});
}

TEST_F(OrderBookTest, TestFillOrderMultipleLevels) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.1");
  processOrderSuccess(book, "102,N,A002,S,2000,10.2");
  // Fully fill A001; Partially fill A002
  processOrderSuccess(book, "102,N,A003,B,3000,10.2");

  assertBookTopPriceLevels(book, {{"10.2", 1000}}, {});
}

TEST_F(OrderBookTest, TestFillOrderMultipleLevelsWithRemaining) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.1");
  processOrderSuccess(book, "102,N,A002,S,2000,10.2");
  // Fully fill A001 and A002
  processOrderSuccess(book, "102,N,A003,B,5000,10.2");

  assertBookTopPriceLevels(book, {}, {{"10.2", 1000}});
}

TEST_F(OrderBookTest, TestAmendCannotChangeSidesS2B) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  // The amend action does not allow for the order to change sides of the
  // orderbook
  processOrderFailure(book, "102,A,A001,B,3000,10.1");

  assertBookTopPriceLevels(book, {{"10.2", 2000}}, {});
}

TEST_F(OrderBookTest, TestAmendCannotChangeSidesB2S) {
  processOrderSuccess(book, "102,N,A001,B,2000,10.2");
  // The amend action does not allow for the order to change sides of the
  // orderbook
  processOrderFailure(book, "102,A,A001,S,3000,10.1");

  assertBookTopPriceLevels(book, {}, {{"10.2", 2000}});
}

TEST_F(OrderBookTest, TestAmendNoChange) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  processOrderSuccess(book, "102,A,A001,S,2000,10.2");
  assertBookTopPriceLevels(book, {{"10.2", 2000}}, {});
}

TEST_F(OrderBookTest, TestAmendSimple) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  processOrderSuccess(book, "102,A,A001,S,3000,10.2");

  assertBookTopPriceLevels(book, {{"10.2", 3000}}, {});
}

TEST_F(OrderBookTest, TestAmendNoCrossing) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  processOrderSuccess(book, "102,N,A002,B,2000,10.1");
  // Amend A002 to 10.0
  processOrderSuccess(book, "102,A,A002,B,2000,10.0");

  assertBookTopPriceLevels(book, {{"10.2", 2000}}, {{"10.0", 2000}});
}

TEST_F(OrderBookTest, TestAmendCrossing) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  processOrderSuccess(book, "102,N,A002,B,2000,10.1");
  // Amend A002 to 10.2, should cross with A001
  processOrderSuccess(book, "102,A,A002,B,2000,10.2");

  assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestAmendThenFullFill) {
  processOrderSuccess(book, "102,N,A003,S,2000,10.3");
  // Still resting after an amendment
  processOrderSuccess(book, "102,A,A003,S,2000,10.2");
  processOrderSuccess(book, "102,N,A004,B,2000,10.2");

  assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestAmendThenPartialFill) {
  processOrderSuccess(book, "102,N,A001,S,1000,10.2");
  // Amend A001 to a bigger quantity
  processOrderSuccess(book, "102,A,A001,S,5000,10.2");
  // A001 partially filled
  processOrderSuccess(book, "102,N,A002,B,2000,10.2");

  assertBookTopPriceLevels(book, {{"10.2", 3000}}, {});
}

TEST_F(OrderBookTest, TestAmendAfterFullyFilled) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  processOrderSuccess(book, "102,N,A002,B,2000,10.2");
  // Cannot amend a fully filled order
  processOrderFailure(book, "102,A,A002,B,2000,10.2");

  assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestAmendMultipleLevels) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.1");
  processOrderSuccess(book, "102,N,A002,S,2000,10.2");
  // Fully fill A001; Partially fill A002
  processOrderSuccess(book, "102,N,A003,B,3000,10.2");

  assertBookTopPriceLevels(book, {{"10.2", 1000}}, {});

  // Amend remaining A002
  processOrderSuccess(book, "102,A,A002,S,3000,10.2");

  assertBookTopPriceLevels(book, {{"10.2", 3000}}, {});
}

TEST_F(OrderBookTest, TestMultiActionSequence) {
  processOrderSuccess(book, "102,N,A001,S,10000,10.5");
  // Partially fill A001
  processOrderSuccess(book, "102,N,B001,B,5000,10.5");

  assertBookTopPriceLevels(book, {{"10.5", 5000}}, {});

  // Amend A001: new price 10.4, new qty 2000
  processOrderSuccess(book, "102,A,A001,S,2000,10.4");

  assertBookTopPriceLevels(book, {{"10.4", 2000}}, {});

  processOrderSuccess(book, "102,C,A001,S,2000,10.4");
  assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestCancelNonExistent) {
  // Remove Order by a key which does not exist
  processOrderFailure(book, "102,C,A001,S,2000,10.2");
}

TEST_F(OrderBookTest, TestCancelOrder) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  processOrderSuccess(book, "102,C,A001,S,2000,10.2");

  assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestCancelIgnoresGivenQuantityAndPrice) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  // Attempt to cancel 5000 when only 2000 had been ordered
  processOrderSuccess(book, "102,C,A001,S,5000,10.2");

  assertBookTopPriceLevels(book, {}, {});

  processOrderSuccess(book, "102,N,A002,S,2000,10.2");
  // Attempt to cancel 2000 at a price of 3.2 when the original order was 10.2
  processOrderSuccess(book, "102,C,A002,S,2000,3.1");

  assertBookTopPriceLevels(book, {}, {});

  processOrderSuccess(book, "102,N,A002,S,5000,10.2");
  // Attempt to cancel with incorrect price and quantity
  processOrderSuccess(book, "102,C,A002,S,2000,3.1");

  assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestCancelOfPartiallyFilled) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  processOrderSuccess(book, "102,N,A002,S,5000,10.2");
  // Partially fill A001
  processOrderSuccess(book, "102,N,A003,B,1000,10.2");
  // Attempt to cancel 2000 when 1000 has been partially filled; Only 1000 will
  // be cancelled
  processOrderSuccess(book, "102,C,A001,S,2000,10.2");

  assertBookTopPriceLevels(book, {{"10.2", 5000}}, {});
}

TEST_F(OrderBookTest, TestCancelAfterFullyFilled) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  processOrderSuccess(book, "102,N,A002,B,2000,10.2");
  // Cannot cancel a fully filled order
  processOrderFailure(book, "102,C,A002,B,2000,10.2");

  assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestDuplicateOrderID) {
  processOrderSuccess(book, "102,N,A001,S,2000,10.2");
  // Order with duplicate order ID
  processOrderFailure(book, "102,N,A001,S,2000,10.2");
}

TEST_F(OrderBookTest, TestCancelledIDReusability) {
  processOrderSuccess(book, "102,N,A001,S,1000,10.1");
  processOrderSuccess(book, "102,C,A001,S,1000,10.1");
  // A001 should be a valid ID
  processOrderSuccess(book, "102,N,A001,S,1000,10.1");

  assertBookTopPriceLevels(book, {{"10.1", 1000}}, {});
}

TEST_F(OrderBookTest, TestFilledIDReusability) {
  processOrderSuccess(book, "102,N,A001,S,1000,10.1");
  processOrderSuccess(book, "102,N,A002,B,1000,10.1");
  // A001 should be a valid ID
  processOrderSuccess(book, "102,N,A001,S,1000,10.1");

  assertBookTopPriceLevels(book, {{"10.1", 1000}}, {});
}
