#pragma once

#include "ui/dashboard.hpp"
#include "ui/dashboard_types.hpp"

namespace ui {

class TickerNotebook {
public:
  TickerNotebook();
  void Render(ui::OrderBookSnapshot &m_snapshot);

private:
  ui::OrderBookPanel m_obp;
};

} // namespace ui
