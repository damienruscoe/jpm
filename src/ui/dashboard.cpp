#include "ui/dashboard.hpp"

#include <algorithm>
#include <cstdio>

#include "imgui.h"
#include "imgui_internal.h"

namespace ui {

namespace {

// --- Bloomberg-style palette -------------------------------------------
constexpr ImVec4 kColorBackground{0.05f, 0.05f, 0.06f, 1.00f};
// constexpr ImVec4 kColorPanelBorder{0.20f, 0.20f, 0.22f, 1.00f};

constexpr ImVec4 kColorBidText{0.20f, 0.80f, 0.35f, 1.00f};
constexpr ImVec4 kColorBidBar{0.10f, 0.35f, 0.16f, 0.55f};
constexpr ImVec4 kColorBidBarBest{0.14f, 0.50f, 0.22f, 0.75f};

constexpr ImVec4 kColorAskText{0.90f, 0.25f, 0.25f, 1.00f};
constexpr ImVec4 kColorAskBar{0.40f, 0.10f, 0.10f, 0.55f};
constexpr ImVec4 kColorAskBarBest{0.60f, 0.14f, 0.14f, 0.75f};

constexpr ImVec4 kColorAmberLabel{0.88f, 0.62f, 0.10f, 1.00f};
constexpr ImVec4 kColorSpreadTight{0.30f, 0.90f, 0.90f, 1.00f};
constexpr ImVec4 kColorSpreadWide{0.95f, 0.75f, 0.25f, 1.00f};
constexpr ImVec4 kColorGreeksText{0.58f, 0.66f, 0.76f, 1.00f};
constexpr ImVec4 kColorMuted{0.45f, 0.45f, 0.48f, 1.00f};
// constexpr ImVec4 kColorOrderId{0.70f, 0.70f, 0.72f, 1.00f};

struct RectPoints {
  ImVec2 top_left;
  ImVec2 bottom_right;
};

// Draws a translucent depth bar filling `fraction` of the current cell,
// anchored to the given side, then leaves the cursor for text on top.
void DrawRect(const RectPoints &rect, float fraction, ImVec4 color,
              bool anchor_right) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  fraction = std::clamp(fraction, 0.0f, 1.0f);
  float bar_width = (rect.bottom_right.x - rect.top_left.x) * fraction;
  ImVec2 bar_min =
      anchor_right ? ImVec2(rect.bottom_right.x - bar_width, rect.top_left.y)
                   : rect.top_left;
  ImVec2 bar_max =
      anchor_right ? rect.bottom_right
                   : ImVec2(rect.top_left.x + bar_width, rect.bottom_right.y);

  draw_list->AddRectFilled(bar_min, bar_max,
                           ImGui::ColorConvertFloat4ToU32(color));
}

RectPoints GetCurrentTableCellContentRect() {
  const float border_size = 1.f;
  RectPoints result;

  ImVec2 cell_padding = ImGui::GetStyle().CellPadding;
  cell_padding.x -= border_size;
  cell_padding.y -= border_size + 1;

  result.top_left = ImGui::GetCursorScreenPos();
  result.top_left.x -= cell_padding.x;
  result.top_left.y -= cell_padding.y;

  result.bottom_right = ImGui::GetCursorScreenPos();
  result.bottom_right.x += cell_padding.x + ImGui::GetContentRegionAvail().x;
  result.bottom_right.y +=
      cell_padding.y + ImGui::GetTextLineHeightWithSpacing() - 2;

  return result;
}

void DrawSizeBar(size_t size, size_t max_size, ImVec4 color,
                 bool anchor_right) {
  float fraction = max_size > 0
                       ? static_cast<float>(size) / static_cast<float>(max_size)
                       : 0.0f;

  RectPoints rect = GetCurrentTableCellContentRect();
  DrawRect(rect, fraction, color, anchor_right);
}

std::string FormatGreeksCompact(const Greeks &greeks) {
  char buffer[96];
  std::snprintf(buffer, sizeof(buffer), "d%.2f g%.3f t%.2f v%.2f r%.2f",
                greeks.delta, greeks.gamma, greeks.theta, greeks.vega,
                greeks.rho);
  return buffer;
}

double BestBid(const OrderBookSnapshot &snapshot) {
  return snapshot.l2_bids.empty() ? 0.0 : snapshot.l2_bids.back().price;
}

double BestAsk(const OrderBookSnapshot &snapshot) {
  return snapshot.l2_asks.empty() ? 0.0 : snapshot.l2_asks.front().price;
}

double Spread(const OrderBookSnapshot &snapshot) {
  double bid = BestBid(snapshot);
  double ask = BestAsk(snapshot);
  if (bid <= 0.0 || ask <= 0.0)
    return 0.0;
  return ask - bid;
}

double SpreadBps(const OrderBookSnapshot &snapshot) {
  double mid = (BestBid(snapshot) + BestAsk(snapshot)) * 0.5;
  if (mid <= 0.0)
    return 0.0;
  return (Spread(snapshot) / mid) * 10000.0;
}

uint64_t MaxLevelSize(auto begin, auto end, int max_depth) {
  uint64_t max_size = 0;
  for (; begin != end; ++begin) {
    max_size = std::max(max_size, begin->size);
    if (--max_depth == 0)
      return max_size;
  }
  return max_size;
}

} // namespace

void OrderBookPanel::Render(const OrderBookSnapshot &snapshot) {
  ImGui::PushStyleColor(ImGuiCol_WindowBg, kColorBackground);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kColorBackground);

  ImGui::BeginChild("OrderBookPanel", ImVec2(0, 0), true);

  RenderToolbar(snapshot);
  ImGui::Separator();
  RenderHeaderBar(snapshot);
  ImGui::Separator();

  RenderLadder(snapshot);

  ImGui::EndChild();
  ImGui::PopStyleColor(2);
}

void OrderBookPanel::RenderToolbar(const OrderBookSnapshot &snapshot) {
  ImGui::TextColored(kColorAmberLabel, "%s",
                     snapshot.symbol.empty() ? "-" : snapshot.symbol.c_str());
  ImGui::SameLine();
  ImGui::TextColored(kColorMuted, "| seq %llu",
                     static_cast<unsigned long long>(snapshot.sequence_number));

  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220.0f);
  ImGui::Checkbox("Greeks", &m_show_greeks);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(100.0f);
  ImGui::SliderInt("Depth", &m_visible_depth, 1, 200);
}

void OrderBookPanel::RenderHeaderBar(const OrderBookSnapshot &snapshot) {
  ImGui::BeginTable("HeaderBar", 3, ImGuiTableFlags_SizingStretchSame);
  ImGui::TableNextRow();

  ImGui::TableSetColumnIndex(0);
  ImGui::TextColored(kColorAmberLabel, "BID");
  ImGui::TextColored(kColorBidText, "%.4f", BestBid(snapshot));

  const double spread_bps = SpreadBps(snapshot);
  ImGui::TableSetColumnIndex(1);
  ImGui::TextColored(kColorAmberLabel, "SPREAD");
  ImGui::TextColored(spread_bps <= 5.0 ? kColorSpreadTight : kColorSpreadWide,
                     "%.4f  (%.1f bps)", Spread(snapshot), spread_bps);

  ImGui::TableSetColumnIndex(2);
  ImGui::TextColored(kColorAmberLabel, "ASK");
  ImGui::TextColored(kColorAskText, "%.4f", BestAsk(snapshot));

  ImGui::EndTable();
}

bool setup_table(const char *title, bool show_greeks) {
  const int column_count = show_greeks ? 8 : 6;
  const ImGuiTableFlags flags =
      ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
      ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;

  auto setup_column = [](const char *header, float width) {
    ImGui::TableSetupColumn(header, ImGuiTableColumnFlags_WidthStretch, width);
  };

  if (!ImGui::BeginTable(title, column_count, flags, ImVec2(0, 0)))
    return false;

  if (show_greeks)
    setup_column("Bid Greeks", 1.6f);
  setup_column("Bid Orders", 0.7f);
  setup_column("Bid Size", 1.0f);
  setup_column("Bid Price", 1.0f);

  setup_column("Ask Price", 1.0f);
  setup_column("Ask Size", 1.0f);
  setup_column("Ask Orders", 0.7f);
  if (show_greeks)
    setup_column("Ask Greeks", 1.6f);

  ImGui::TableHeadersRow();
  return true;
}

void OrderBookPanel::RenderLadder(const OrderBookSnapshot &snapshot) {
  uint64_t max_bid_size = MaxLevelSize(
      snapshot.l2_bids.rbegin(), snapshot.l2_bids.rend(), m_visible_depth);
  uint64_t max_ask_size = MaxLevelSize(snapshot.l2_asks.begin(),
                                       snapshot.l2_asks.end(), m_visible_depth);
  uint64_t max_size = std::max(max_bid_size, max_ask_size);

  if (!setup_table("L2Ladder", m_show_greeks))
    return;

  for (int row_idx = 0; row_idx < m_visible_depth; ++row_idx) {
    ImGui::TableNextRow(ImGuiTableRowFlags_None,
                        ImGui::GetTextLineHeightWithSpacing());

    bool has_bid = row_idx < static_cast<int>(snapshot.l2_bids.size());
    bool has_ask = row_idx < static_cast<int>(snapshot.l2_asks.size());

    int col = 0;

    if (has_bid) {
      const auto &row = snapshot.l2_bids[snapshot.l2_bids.size() - row_idx - 1];

      if (m_show_greeks) {
        ImGui::TableSetColumnIndex(col++);
        ImGui::TextColored(kColorGreeksText, "%s",
                           FormatGreeksCompact(row.greeks).c_str());
      }

      ImGui::TableSetColumnIndex(col++);
      ImGui::TextColored(kColorMuted, "%u", row.order_count);

      ImGui::TableSetColumnIndex(col++);
      DrawSizeBar(row.size, max_size,
                  row_idx == 0 ? kColorBidBarBest : kColorBidBar, true);
      ImGui::TextColored(kColorBidText, "%llu",
                         static_cast<unsigned long long>(row.size));

      ImGui::TableSetColumnIndex(col++);
      ImGui::TextColored(kColorBidText, "%.4f", row.price);
    } else {
      col += m_show_greeks ? 4 : 3;
    }

    if (has_ask) {
      const auto &row = snapshot.l2_asks[row_idx];

      ImGui::TableSetColumnIndex(col++);
      ImGui::TextColored(kColorAskText, "%.4f", row.price);

      ImGui::TableSetColumnIndex(col++);
      DrawSizeBar(row.size, max_size,
                  row_idx == 0 ? kColorAskBarBest : kColorAskBar, false);
      ImGui::TextColored(kColorAskText, "%llu",
                         static_cast<unsigned long long>(row.size));

      ImGui::TableSetColumnIndex(col++);
      ImGui::TextColored(kColorMuted, "%u", row.order_count);

      if (m_show_greeks) {
        ImGui::TableSetColumnIndex(col++);
        ImGui::TextColored(kColorGreeksText, "%s",
                           FormatGreeksCompact(row.greeks).c_str());
      }
    } else {
      col += m_show_greeks ? 4 : 3;
    }
  }

  ImGui::EndTable();
}

} // namespace ui
