#include "../src/fixed_point.hpp"
#include "../src/order_book.hpp"
#include "../src/order_id.hpp"
#include <gtest/gtest.h>

#include "../src/signals/cumulative_vwap.hpp"
#include "../src/signals/ema_signal.hpp"
#include "../src/signals/last_trade_price.hpp"
#include "../src/signals/static_composite.hpp"

class CumulativeMetricsTest : public testing::Test {
protected:
  using Price = FixedPoint<4>;
  using Quantity = uint32_t;
  using Traits = OrderBookTraits<FixedSizeOrderID, Price, Quantity>;
  using SignalAgregator =
      signals::StaticComposite<Traits, signals::EmaSignal<Traits>,
                               signals::LastTradePrice<Traits>,
                               signals::CumulativeVWAP<Traits>>;

  using Book = OrderBook<Traits, SignalAgregator>;
  Book book;
};

TEST_F(CumulativeMetricsTest, TestSignalMetrics) {
  // Initially, metrics should be null
  ASSERT_EQ(book.getSignals().getLastTradedPrice(), std::nullopt);
  ASSERT_EQ(book.getSignals().getLastTradedQuantity(), std::nullopt);
  ASSERT_EQ(book.getSignals().getCumulativeVolume(), std::nullopt);
  ASSERT_EQ(book.getSignals().getCumulativeValue(), std::nullopt);
  ASSERT_EQ(book.getSignals().getCumulativeVwap(), std::nullopt);
  ASSERT_EQ(book.getSignals().getEmaPrice(), std::nullopt);
  ASSERT_EQ(book.getSignals().getEmaVwap(), std::nullopt);

  // New order that doesn't trade
  (void)book.newOrder("A001", side_t::ASK, *Price::Parse("10.0"), 100);
  ASSERT_EQ(book.getSignals().getLastTradedPrice(), std::nullopt);
  ASSERT_EQ(book.getSignals().getLastTradedQuantity(), std::nullopt);
  ASSERT_EQ(book.getSignals().getCumulativeVolume(), std::nullopt);
  ASSERT_EQ(book.getSignals().getCumulativeValue(), std::nullopt);
  ASSERT_EQ(book.getSignals().getCumulativeVwap(), std::nullopt);
  ASSERT_EQ(book.getSignals().getEmaPrice(), std::nullopt);
  ASSERT_EQ(book.getSignals().getEmaVwap(), std::nullopt);

  // New order that trades: BID 10.0, 50.
  (void)book.newOrder("A002", side_t::BID, *Price::Parse("10.0"), 50);

  ASSERT_EQ(book.getSignals().getLastTradedPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getSignals().getLastTradedQuantity(), 50);
  ASSERT_EQ(book.getSignals().getCumulativeVolume(), *Price::Parse("50"));
  ASSERT_EQ(book.getSignals().getCumulativeValue(), *Price::Parse("500"));
  ASSERT_EQ(book.getSignals().getCumulativeVwap(), *Price::Parse("10"));
  ASSERT_EQ(book.getSignals().getEmaPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getSignals().getEmaVwap(), *Price::Parse("10.0"));

  // Another Ask, without a triggering a trade
  (void)book.newOrder("A003", side_t::ASK, *Price::Parse("12.0"), 12);

  ASSERT_EQ(book.getSignals().getLastTradedPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getSignals().getLastTradedQuantity(), 50);
  ASSERT_EQ(*book.getSignals().getCumulativeVolume(), *Price::Parse("50"));
  ASSERT_EQ(*book.getSignals().getCumulativeValue(), *Price::Parse("500"));
  ASSERT_EQ(book.getSignals().getCumulativeVwap(), *Price::Parse("10"));
  ASSERT_EQ(book.getSignals().getEmaPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getSignals().getEmaVwap(), *Price::Parse("10.0"));

  // Another trade: BID 10.0, 10.
  (void)book.newOrder("A004", side_t::BID, *Price::Parse("10.0"), 10);

  ASSERT_EQ(book.getSignals().getLastTradedPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getSignals().getLastTradedQuantity(), 10);
  ASSERT_EQ(*book.getSignals().getCumulativeVolume(), *Price::Parse("60"));
  ASSERT_EQ(*book.getSignals().getCumulativeValue(), *Price::Parse("600"));
  ASSERT_EQ(book.getSignals().getCumulativeVwap(), *Price::Parse("10"));
  ASSERT_EQ(book.getSignals().getEmaPrice(), *Price::Parse("10.0"));
  ASSERT_EQ(book.getSignals().getEmaVwap(), *Price::Parse("10.0"));
}
