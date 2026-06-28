#include "../src/fixed_point.hpp"
#include "../src/order_book.hpp"
#include "../src/order_id.hpp"
#include "../src/signals/dynamic_composite.hpp"
#include "../src/signals/ema_signal.hpp"
#include <gtest/gtest.h>

class SimulationSignalsTest : public ::testing::Test {
protected:
  using Price = FixedPoint<4>;
  using Quantity = uint32_t;
  using Traits = OrderBookTraits<FixedSizeOrderID, Price, Quantity>;
  using Aggregator = signals::DynamicComposite<Traits>;
  using Book = OrderBook<Traits, Aggregator>;

  Book book{};
};

TEST_F(SimulationSignalsTest, MultipleSignalsPolymorphicUpdate) {
  signals::EmaSignal<Traits> emaSignal;
  ASSERT_EQ(emaSignal.getEmaVwap(), std::nullopt);

  // Trade
  (void)book.newOrder("A001", side_t::BID, *Price::Parse("10.0"), 100);
  (void)book.newOrder("A002", side_t::ASK, *Price::Parse("10.0"), 100);

  // Register signal
  book.getSignals().addSignal(&emaSignal);
  ASSERT_EQ(emaSignal.getEmaVwap(), std::nullopt);

  // Trade
  (void)book.newOrder("A003", side_t::BID, *Price::Parse("10.0"), 100);
  (void)book.newOrder("A004", side_t::ASK, *Price::Parse("10.0"), 100);

  // Verify signal updated
  ASSERT_TRUE(emaSignal.getEmaVwap().has_value());
  ASSERT_EQ(*emaSignal.getEmaVwap(), *Price::Parse("10.0"));
}
