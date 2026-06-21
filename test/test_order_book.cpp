#include <ranges>
#include <gtest/gtest.h>
#include "../src/parser.hpp"
#include "../src/order_book.hpp"
#include "../src/FixedPoint.hpp"

struct Level {
  using OrderID = std::string;
  using Price = FixedPointAI;
  using Quantity = uint32_t;

  friend std::ostream &operator<<(std::ostream &os, const Level &level) {
    return os << level.price << ' ' << level.quantity;
  }

  auto operator<=>(const Level &) const = default;

  Price price{};
  Quantity quantity{};
};

class OrderBookTest : public testing::Test {
 protected:
  OrderBookTest() {
  }

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

side_t convertSide(Side side)
{
	return side == Side::Buy ? side_t::BID : side_t::ASK;
}

template<typename OrderBook>
void process(OrderBook& book, std::string_view dsl_add_order)
{
	bool success = false;
	auto msg = parse_line(dsl_add_order);
	switch (msg->type) {
		case RequestType::New: {
			success = book.newOrder(msg->order_id, convertSide(msg->side), msg->price, msg->quantity);
			break;
		}
		case RequestType::Cancel: {
			success = book.cancel(msg->order_id, convertSide(msg->side));
			break;
		}
		case RequestType::Amend: {
			success = book.amend(msg->order_id, convertSide(msg->side), msg->price, msg->quantity);
			break;
		}
	}
	ASSERT_TRUE(success);
}


template <typename Price>
void assertTopOfBook(const auto& top_of_book, const auto& expected)
{
		ASSERT_EQ(top_of_book.size(), expected.size());
		size_t index = 0;
		for (const auto& [price, quantity]: expected) {
			ASSERT_EQ(top_of_book[index].price, Price::Parse(price));
			ASSERT_EQ(top_of_book[index].quantity, quantity);
			++index;
		}
}

using ExpectedTopOfBook = std::vector<std::pair<std::string, uint64_t>>;

template <typename OrderBook>
void assertBookTopPriceLevels(const OrderBook& book, const ExpectedTopOfBook& expected1, const ExpectedTopOfBook& expected2)
{
	auto reversed = ExpectedTopOfBook(expected1.rbegin(), expected1.rend());
	assertTopOfBook<typename OrderBook::Price>(book.getTopAsk(10), reversed);
	assertTopOfBook<typename OrderBook::Price>(book.getTopBid(10), expected2);
}
	
TEST_F(OrderBookTest, GivenExampleMarket1) {
	process(book, "101,N,A001,S,3000,6.8");
	assertBookTopPriceLevels(book, { { "6.8", 3000 } }, {});

	process(book, "101,N,A002,S,1000,6.9");
	assertBookTopPriceLevels(book, { { "6.9", 1000 }, { "6.8", 3000 } }, {});

	process(book, "101,N,A003,B,2000,6.7");
	assertBookTopPriceLevels(book, { { "6.9", 1000 }, { "6.8", 3000 } }, { { "6.7", 2000 } });

	process(book, "101,N,A004,B,1000,6.8 // crosses with order A001 with price 6.8");
	assertBookTopPriceLevels(book, { { "6.9", 1000 }, { "6.8", 2000 } }, { { "6.7", 2000 } });

	process(book, "101,A,A003,B,2000,6.9 // after amending A003 crosses with A001 and both fully filled");
	assertBookTopPriceLevels(book, { { "6.9", 1000 } }, {});
}

TEST_F(OrderBookTest, GivenExampleMarket2) {
	process(book, "102,N,A005,S,2000,10.2");
	assertBookTopPriceLevels(book, { { "10.2", 2000 } }, {});

	process(book, "102,N,A006,B,2000,10.1");
	assertBookTopPriceLevels(book, { { "10.2", 2000 } }, { { "10.1", 2000 } });

	process(book, "102,N,A007,B,2000,10.1");
	assertBookTopPriceLevels(book, { { "10.2", 2000 } }, { { "10.1", 4000 } });

	process(book, "102,C,A006,B,2000,10.1 // A006 will be removed from order book");
	assertBookTopPriceLevels(book, { { "10.2", 2000 } }, { { "10.1", 2000 } });
}

TEST_F(OrderBookTest, TestFillOrder) {
	process(book, "102,N,A001,S,5000,10.2");
	process(book, "102,N,A002,B,2000,10.2");

	assertBookTopPriceLevels(book, { { "10.2", 3000 } }, {});
}

TEST_F(OrderBookTest, TestFillOrderWithRemaining) {
	process(book, "102,N,A001,S,2000,10.2");
	process(book, "102,N,A002,B,5000,10.2 // Fully fill A001");

	assertBookTopPriceLevels(book, {}, { { "10.2", 3000 } });
}

TEST_F(OrderBookTest, TestFillOrderMultipleLevels) {
	process(book, "102,N,A001,S,2000,10.1");
	process(book, "102,N,A002,S,2000,10.2");
	process(book, "102,N,A003,B,3000,10.2 // Fully fill A001; Partially fill A002");

	assertBookTopPriceLevels(book, { { "10.2", 1000 } }, {});
}

TEST_F(OrderBookTest, TestFillOrderMultipleLevelsWithRemaining) {
	process(book, "102,N,A001,S,2000,10.1");
	process(book, "102,N,A002,S,2000,10.2");
	process(book, "102,N,A003,B,5000,10.2 // Fully fill A001 and A002");

	assertBookTopPriceLevels(book, {}, { { "10.2", 1000 } });
}

TEST_F(OrderBookTest, TestCancelOfPartiallyFilled) {
	process(book, "102,N,A001,S,2000,10.2");
	process(book, "102,N,A002,S,5000,10.2");
	process(book, "102,N,A003,B,1000,10.2 // Partially fill A001");
	process(book, "102,C,A001,S,2000,10.2 // Attempt to cancel 2000 when 1000 has been partially filled; Only 1000 will be cancelled");

	assertBookTopPriceLevels(book, { { "10.2", 5000 } }, {});
}

TEST_F(OrderBookTest, TestDuplicateOrderID) {
	process(book, "102,N,A001,S,2000,10.2");
	auto msg = parse_line("102,N,A001,S,2000,10.2");
	bool success = book.newOrder(msg->order_id, convertSide(msg->side), msg->price, msg->quantity);
	ASSERT_FALSE(success);
}

TEST_F(OrderBookTest, TestCancelNonExistent) {
	bool success = book.cancel("NONEXISTENT", side_t::BID);
	ASSERT_FALSE(success);
}

TEST_F(OrderBookTest, TestAmendNoChange) {
	process(book, "102,N,A001,S,2000,10.2");
	process(book, "102,A,A001,S,2000,10.2");
	assertBookTopPriceLevels(book, { { "10.2", 2000 } }, {});
}

TEST_F(OrderBookTest, TestAmendSimple) {
	process(book, "102,N,A001,S,2000,10.2");
	process(book, "102,A,A001,S,3000,10.2");
	assertBookTopPriceLevels(book, { { "10.2", 3000 } }, {});
}

TEST_F(OrderBookTest, TestAmendNoCrossing) {
	process(book, "102,N,A001,S,2000,10.2");
	process(book, "102,N,A002,B,2000,10.1");
	process(book, "102,A,A002,B,2000,10.0 // Amend A002 to 10.0");
	
	assertBookTopPriceLevels(book, { { "10.2", 2000 } }, { { "10.0", 2000 } });
}

TEST_F(OrderBookTest, TestAmendCrossing) {
	process(book, "102,N,A001,S,2000,10.2");
	process(book, "102,N,A002,B,2000,10.1");
	process(book, "102,A,A002,B,2000,10.2 // Amend A002 to 10.2, should cross with A001");
	
	assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestAmendFullFill) {
	process(book, "102,N,A003,S,2000,10.3");
	process(book, "102,A,A003,S,2000,10.2 // Still resting after an amendment");
	process(book, "102,N,A004,B,2000,10.2");
	assertBookTopPriceLevels(book, {}, {});
}

TEST_F(OrderBookTest, TestAmendPartialFill) {
	process(book, "102,N,A001,S,5000,10.2");
	process(book, "102,A,A001,S,5000,10.2 // No change");
	process(book, "102,N,A002,B,2000,10.2 // A001 partially filled");
	assertBookTopPriceLevels(book, { { "10.2", 3000 } }, {});
}

TEST_F(OrderBookTest, TestAmendMultipleLevels) {
	process(book, "102,N,A001,S,2000,10.1");
	process(book, "102,N,A002,S,2000,10.2");
	process(book, "102,N,A003,B,3000,10.2 // Fully fill A001; Partially fill A002");

	assertBookTopPriceLevels(book, { { "10.2", 1000 } }, {});

	process(book, "102,A,A002,S,3000,10.2 // Amend remaining A002");

	assertBookTopPriceLevels(book, { { "10.2", 3000 } }, {});
}

