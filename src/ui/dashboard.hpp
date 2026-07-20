#pragma once

#include "dashboard_types.hpp"

namespace ui {

class OrderBookPanel {
public:
  void Render(const OrderBookSnapshot &snapshot);

  void SetVisibleDepth(int depth) { m_visible_depth = depth; }
  int GetVisibleDepth() const { return m_visible_depth; }

  void SetShowGreeks(bool show) { m_show_greeks = show; }
  bool GetShowGreeks() const { return m_show_greeks; }

private:
  void RenderToolbar(const OrderBookSnapshot &snapshot);
  void RenderHeaderBar(const OrderBookSnapshot &snapshot);
  void RenderLadder(const OrderBookSnapshot &snapshot);

  int m_visible_depth = 200;
  bool m_show_greeks = false;
};

} // namespace ui
