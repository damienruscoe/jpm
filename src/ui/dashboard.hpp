#pragma once

#include "dashboard_types.hpp"

namespace ui {

// Renders a Bloomberg-style depth ladder: a header strip with best
// bid / spread / best ask, and a two-sided price ladder below with
// per-level Greeks. Stateless w.r.t. market data — call Render()
// every frame with the latest snapshot. Owns only display state
// (mode, depth, toggles).
class OrderBookPanel {
public:
  void Render(const OrderBookSnapshot &snapshot);

  void SetMode(BookMode mode) { m_mode = mode; }
  BookMode GetMode() const { return m_mode; }

  void SetVisibleDepth(int depth) { m_visible_depth = depth; }
  int GetVisibleDepth() const { return m_visible_depth; }

  void SetShowGreeks(bool show) { m_show_greeks = show; }
  bool GetShowGreeks() const { return m_show_greeks; }

private:
  void RenderToolbar(const OrderBookSnapshot &snapshot);
  void RenderHeaderBar(const OrderBookSnapshot &snapshot);
  void RenderL2Ladder(const OrderBookSnapshot &snapshot);
  void RenderL3Ladder(const OrderBookSnapshot &snapshot);

  static double BestBid(const OrderBookSnapshot &snapshot);
  static double BestAsk(const OrderBookSnapshot &snapshot);
  static double Spread(const OrderBookSnapshot &snapshot);
  static double SpreadBps(const OrderBookSnapshot &snapshot);
  static uint64_t MaxLevelSize(const std::vector<L2Level> &levels);
  static uint64_t MaxLevelSize(const std::vector<L3Level> &levels);

  BookMode m_mode = BookMode::L3;
  int m_visible_depth = 200;
  bool m_show_greeks = false;
  bool m_expand_l3 = false;
};

} // namespace ui
