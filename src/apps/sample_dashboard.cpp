#include "ui/root.hpp"

#include <iostream>

unsigned bounded_rand(unsigned range) {
  for (unsigned x, r;;)
    if (x = rand(), r = x % range, x - r <= -range)
      return r + 1;
}

double mid_price = 100.0;

ui::OrderBookSnapshot make_fake_data() {
  ui::OrderBookSnapshot ss;
  ss.mode = ui::BookMode::L3;
  const auto FakeGreeks =
      ui::Greeks{0.12345, 0.234568, 0.000000123, 0.76920348, 0.0012379};
  ss.symbol = "APPL";

  const double adj = static_cast<double>(bounded_rand(240)) - 120.0;
  mid_price += (adj / 1000.0);

  for (unsigned n = 1; n < 30 + bounded_rand(3); ++n)
    ss.l3_bids.push_back({mid_price - (0.1 * n),
                          [&]() {
                            std::vector<ui::L3Order> result;
                            for (unsigned m = 0; m < bounded_rand(n); ++m)
                              result.push_back({0, mid_price - (0.1 * n),
                                                bounded_rand(30 + (50 * m)), 0,
                                                FakeGreeks});
                            return result;
                          }(),
                          FakeGreeks});
  for (unsigned n = 1; n < 17 + bounded_rand(5); ++n) {
    ss.l3_asks.push_back({mid_price + (0.1 * n),
                          [&]() {
                            std::vector<ui::L3Order> result;
                            for (unsigned m = 0; m < bounded_rand(n); ++m)
                              result.push_back({0, mid_price - (0.1 * n),
                                                bounded_rand(40 + (50 * m)), 0,
                                                FakeGreeks});
                            return result;
                          }(),
                          FakeGreeks});
  }

  for (const auto &level : ss.l3_bids)
    ss.l2_bids.push_back({level.price, level.TotalSize(),
                          static_cast<uint32_t>(level.orders.size()),
                          FakeGreeks});
  for (const auto &level : ss.l3_asks)
    ss.l2_asks.push_back({level.price, level.TotalSize(),
                          static_cast<uint32_t>(level.orders.size()),
                          FakeGreeks});

  return ss;
}

void process_update_queue(ui::OrderBookSnapshot &snapshot) {
  snapshot = make_fake_data();
}

int main() {
  try {
    ui::Root root;
    root.update_queue = &process_update_queue;
    root.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal startup exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception during startup!" << std::endl;
    return 1;
  }

  return 0;
}
