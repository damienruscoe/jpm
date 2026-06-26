#include "../src/fixed_point.hpp"
#include "../src/order_book.hpp"
#include "../src/order_id.hpp"
#include <gtest/gtest.h>

class CumulativeMetricsTest : public testing::Test {
protected:
  using Price = FixedPoint<4>;
  using Quantity = uint32_t;
  using Book = OrderBook<FixedSizeOrderID, Price, Quantity>;
  Book book;
};

TEST_F(CumulativeMetricsTest, TestSignalMetrics) {
  // Initially, metrics should be null
  ASSERT_EQ(book.getCumulativeVolume(), std::nullopt);
  ASSERT_EQ(book.getCumulativeValue(), std::nullopt);
  ASSERT_EQ(book.getCumulativeVwap(), std::nullopt);
  ASSERT_EQ(book.getEmaPrice(), std::nullopt);
  ASSERT_EQ(book.getEmaVwap(), std::nullopt);

  // New order that doesn't trade
  (void)book.newOrder("A001", side_t::ASK, *Price::Parse("10.0"), 100);
  ASSERT_EQ(book.getCumulativeVolume(), std::nullopt);
  ASSERT_EQ(book.getCumulativeValue(), std::nullopt);
  ASSERT_EQ(book.getCumulativeVwap(), std::nullopt);
  ASSERT_EQ(book.getEmaPrice(), std::nullopt);
  ASSERT_EQ(book.getEmaVwap(), std::nullopt);

  // New order that trades: BID 10.0, 50.
  (void)book.newOrder("A002", side_t::BID, *Price::Parse("10.0"), 50);

  ASSERT_EQ(book.getCumulativeVolume(), *Price::Parse("50"));
  ASSERT_EQ(book.getCumulativeValue(), *Price::Parse("500"));
  ASSERT_EQ(book.getCumulativeVwap(), *Price::Parse("10"));
  ASSERT_EQ(book.getEmaPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getEmaVwap(), *Price::Parse("10.0"));

  // Another trade: ASK 10.0, 50.
  (void)book.newOrder("A003", side_t::ASK, *Price::Parse("10.0"), 50);

  ASSERT_EQ(*book.getCumulativeVolume(), *Price::Parse("50"));
  ASSERT_EQ(*book.getCumulativeValue(), *Price::Parse("500"));
  ASSERT_EQ(book.getCumulativeVwap(), *Price::Parse("10"));
  ASSERT_EQ(book.getEmaPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getEmaVwap(), *Price::Parse("10.0"));

  // Another trade: BID 10.0, 10.
  (void)book.newOrder("A004", side_t::BID, *Price::Parse("10.0"), 10);

  ASSERT_EQ(*book.getCumulativeVolume(), *Price::Parse("60"));
  ASSERT_EQ(*book.getCumulativeValue(), *Price::Parse("600"));
  ASSERT_EQ(book.getCumulativeVwap(), *Price::Parse("10"));
  ASSERT_EQ(book.getEmaPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getEmaVwap(), *Price::Parse("10.0"));
}
