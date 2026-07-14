#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ui {

enum class Side : uint8_t { Bid, Ask };

enum class BookMode : uint8_t { L2, L3 };

// Per-price-level (or per-order) option Greeks. Zero-initialised so
// non-derivative instruments can simply leave them unset.
struct Greeks {
  double delta = 0.0;
  double gamma = 0.0;
  double theta = 0.0;
  double vega = 0.0;
  double rho = 0.0;
};

struct L2Level {
  double price = 0.0;
  uint64_t size = 0;
  uint32_t order_count = 0;
  Greeks greeks;
};

// A single resting order at a price level, as seen in an L3 (MBO) feed.
struct L3Order {
  uint64_t order_id = 0;
  double price = 0.0;
  uint64_t size = 0;
  uint64_t timestamp_ns = 0;
  Greeks greeks;
};

// L3 price level: the individual orders that make it up, plus a
// size-weighted aggregate used when the level is collapsed in the UI.
struct L3Level {
  double price = 0.0;
  std::vector<L3Order> orders;
  Greeks aggregate_greeks;

  uint64_t TotalSize() const {
    uint64_t total = 0;
    for (const auto &order : orders)
      total += order.size;
    return total;
  }
};

// Feed contract: producer fills whichever of the L2 / L3 vectors match
// `mode` and hands the snapshot to OrderBookPanel::Render. Both sides
// must arrive sorted best-to-worst, i.e. bids[0] is the highest bid,
// asks[0] is the lowest ask.
struct OrderBookSnapshot {
  std::string symbol;
  BookMode mode = BookMode::L2;

  std::vector<L2Level> l2_bids;
  std::vector<L2Level> l2_asks;

  std::vector<L3Level> l3_bids;
  std::vector<L3Level> l3_asks;

  uint64_t sequence_number = 0;
  uint64_t timestamp_ns = 0;

  bool Empty() const {
    return l2_bids.empty() && l2_asks.empty() && l3_bids.empty() &&
           l3_asks.empty();
  }
};

} // namespace ui
