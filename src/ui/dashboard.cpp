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
constexpr ImVec4 kColorOrderId{0.70f, 0.70f, 0.72f, 1.00f};

// Draws a translucent depth bar filling `fraction` of the current cell,
// anchored to the given side, then leaves the cursor for text on top.
void DrawSizeBar(float fraction, ImVec4 color, bool anchor_right) {
  fraction = std::clamp(fraction, 0.0f, 1.0f);

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  ImVec2 cell_min = ImGui::GetCursorScreenPos();
  ImVec2 cell_max = ImVec2(cell_min.x + ImGui::GetContentRegionAvail().x,
                           cell_min.y + ImGui::GetTextLineHeightWithSpacing());

  float bar_width = (cell_max.x - cell_min.x) * fraction;
  ImVec2 bar_min =
      anchor_right ? ImVec2(cell_max.x - bar_width, cell_min.y) : cell_min;
  ImVec2 bar_max =
      anchor_right ? cell_max : ImVec2(cell_min.x + bar_width, cell_max.y);

  draw_list->AddRectFilled(bar_min, bar_max,
                           ImGui::ColorConvertFloat4ToU32(color));
}

std::string FormatGreeksCompact(const Greeks &greeks) {
  char buffer[96];
  std::snprintf(buffer, sizeof(buffer), "d%.2f g%.3f t%.2f v%.2f r%.2f",
                greeks.delta, greeks.gamma, greeks.theta, greeks.vega,
                greeks.rho);
  return buffer;
}

} // namespace

double OrderBookPanel::BestBid(const OrderBookSnapshot &snapshot) {
  if (snapshot.mode == BookMode::L2)
    return snapshot.l2_bids.empty() ? 0.0 : snapshot.l2_bids.front().price;
  return snapshot.l3_bids.empty() ? 0.0 : snapshot.l3_bids.front().price;
}

double OrderBookPanel::BestAsk(const OrderBookSnapshot &snapshot) {
  if (snapshot.mode == BookMode::L2)
    return snapshot.l2_asks.empty() ? 0.0 : snapshot.l2_asks.front().price;
  return snapshot.l3_asks.empty() ? 0.0 : snapshot.l3_asks.front().price;
}

double OrderBookPanel::Spread(const OrderBookSnapshot &snapshot) {
  double bid = BestBid(snapshot);
  double ask = BestAsk(snapshot);
  if (bid <= 0.0 || ask <= 0.0)
    return 0.0;
  return ask - bid;
}

double OrderBookPanel::SpreadBps(const OrderBookSnapshot &snapshot) {
  // double bid = BestBid(snapshot);
  double mid = (BestBid(snapshot) + BestAsk(snapshot)) * 0.5;
  if (mid <= 0.0)
    return 0.0;
  return (Spread(snapshot) / mid) * 10000.0;
}

uint64_t OrderBookPanel::MaxLevelSize(const std::vector<L2Level> &levels) {
  uint64_t max_size = 0;
  for (const auto &level : levels)
    max_size = std::max(max_size, level.size);
  return max_size;
}

uint64_t OrderBookPanel::MaxLevelSize(const std::vector<L3Level> &levels) {
  uint64_t max_size = 0;
  for (const auto &level : levels)
    max_size = std::max(max_size, level.TotalSize());
  return max_size;
}

void OrderBookPanel::Render(const OrderBookSnapshot &snapshot) {
  ImGui::PushStyleColor(ImGuiCol_WindowBg, kColorBackground);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kColorBackground);

  ImGui::BeginChild("OrderBookPanel", ImVec2(0, 0), true);

  RenderToolbar(snapshot);
  ImGui::Separator();
  RenderHeaderBar(snapshot);
  ImGui::Separator();

  if (snapshot.mode == BookMode::L2)
    RenderL2Ladder(snapshot);
  else
    RenderL3Ladder(snapshot);

  ImGui::EndChild();
  ImGui::PopStyleColor(2);
  // ImGui::PopID();
}

void OrderBookPanel::RenderToolbar(const OrderBookSnapshot &snapshot) {
  ImGui::TextColored(kColorAmberLabel, "%s",
                     snapshot.symbol.empty() ? "-" : snapshot.symbol.c_str());
  ImGui::SameLine();
  ImGui::TextColored(kColorMuted, "| %s | seq %llu",
                     snapshot.mode == BookMode::L2 ? "L2" : "L3",
                     static_cast<unsigned long long>(snapshot.sequence_number));

  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 220.0f);
  ImGui::Checkbox("Greeks", &m_show_greeks);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(100.0f);
  ImGui::SliderInt("Depth", &m_visible_depth, 1, 300);
}

void OrderBookPanel::RenderHeaderBar(const OrderBookSnapshot &snapshot) {
  double bid_px = BestBid(snapshot);
  double ask_px = BestAsk(snapshot);
  double spread_px = Spread(snapshot);
  double spread_bps = SpreadBps(snapshot);

  ImVec4 spread_color =
      spread_bps <= 5.0 ? kColorSpreadTight : kColorSpreadWide;

  ImGui::BeginTable("HeaderBar", 3, ImGuiTableFlags_SizingStretchSame);
  ImGui::TableNextRow();

  ImGui::TableSetColumnIndex(0);
  ImGui::TextColored(kColorAmberLabel, "BID");
  ImGui::TextColored(kColorBidText, "%.4f", bid_px);

  ImGui::TableSetColumnIndex(1);
  ImGui::TextColored(kColorAmberLabel, "SPREAD");
  ImGui::TextColored(spread_color, "%.4f  (%.1f bps)", spread_px, spread_bps);

  ImGui::TableSetColumnIndex(2);
  ImGui::TextColored(kColorAmberLabel, "ASK");
  ImGui::TextColored(kColorAskText, "%.4f", ask_px);

  ImGui::EndTable();
}

void OrderBookPanel::RenderL2Ladder(const OrderBookSnapshot &snapshot) {
  uint64_t max_bid_size = MaxLevelSize(snapshot.l2_bids);
  uint64_t max_ask_size = MaxLevelSize(snapshot.l2_asks);
  uint64_t max_size = std::max(max_bid_size, max_ask_size);

  int column_count = m_show_greeks ? 8 : 6;
  ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                          ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_SizingStretchProp;

  if (!ImGui::BeginTable("L2Ladder", column_count, flags, ImVec2(0, 0)))
    return;

  if (m_show_greeks)
    ImGui::TableSetupColumn("Bid Greeks", ImGuiTableColumnFlags_WidthStretch,
                            1.6f);
  ImGui::TableSetupColumn("Bid Orders", ImGuiTableColumnFlags_WidthStretch,
                          0.7f);
  ImGui::TableSetupColumn("Bid Size", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Bid Price", ImGuiTableColumnFlags_WidthStretch,
                          1.0f);
  ImGui::TableSetupColumn("Ask Price", ImGuiTableColumnFlags_WidthStretch,
                          1.0f);
  ImGui::TableSetupColumn("Ask Size", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Ask Orders", ImGuiTableColumnFlags_WidthStretch,
                          0.7f);
  if (m_show_greeks)
    ImGui::TableSetupColumn("Ask Greeks", ImGuiTableColumnFlags_WidthStretch,
                            1.6f);
  ImGui::TableHeadersRow();

  int row_count = std::min<int>(
      m_visible_depth, static_cast<int>(std::max(snapshot.l2_bids.size(),
                                                 snapshot.l2_asks.size())));

  for (int row = 0; row < row_count; ++row) {
    ImGui::TableNextRow();
    bool has_bid = row < static_cast<int>(snapshot.l2_bids.size());
    bool has_ask = row < static_cast<int>(snapshot.l2_asks.size());

    // const auto* bid_row = has_bid ? &snapshot.l2_bids[row] : nullptr;
    const auto *bid_row =
        has_bid ? &snapshot.l2_bids[snapshot.l2_bids.size() - row - 1]
                : nullptr;
    const auto *ask_row = has_ask ? &snapshot.l2_asks[row] : nullptr;

    bool is_best = row == 0;

    int col = 0;

    if (m_show_greeks) {
      ImGui::TableSetColumnIndex(col++);
      if (bid_row)
        ImGui::TextColored(kColorGreeksText, "%s",
                           FormatGreeksCompact(bid_row->greeks).c_str());
    }

    ImGui::TableSetColumnIndex(col++);
    if (bid_row)
      ImGui::TextColored(kColorMuted, "%u", bid_row->order_count);

    ImGui::TableSetColumnIndex(col++);
    if (bid_row) {
      float fraction = max_size > 0 ? static_cast<float>(bid_row->size) /
                                          static_cast<float>(max_size)
                                    : 0.0f;
      DrawSizeBar(fraction, is_best ? kColorBidBarBest : kColorBidBar, true);
      ImGui::TextColored(kColorBidText, "%llu",
                         static_cast<unsigned long long>(bid_row->size));
    }

    ImGui::TableSetColumnIndex(col++);
    if (bid_row)
      ImGui::TextColored(kColorBidText, "%.4f", bid_row->price);

    ImGui::TableSetColumnIndex(col++);
    if (ask_row)
      ImGui::TextColored(kColorAskText, "%.4f", ask_row->price);

    ImGui::TableSetColumnIndex(col++);
    if (ask_row) {
      float fraction = max_size > 0 ? static_cast<float>(ask_row->size) /
                                          static_cast<float>(max_size)
                                    : 0.0f;
      DrawSizeBar(fraction, is_best ? kColorAskBarBest : kColorAskBar, false);
      ImGui::TextColored(kColorAskText, "%llu",
                         static_cast<unsigned long long>(ask_row->size));
    }

    ImGui::TableSetColumnIndex(col++);
    if (ask_row)
      ImGui::TextColored(kColorMuted, "%u", ask_row->order_count);

    if (m_show_greeks) {
      ImGui::TableSetColumnIndex(col++);
      if (ask_row)
        ImGui::TextColored(kColorGreeksText, "%s",
                           FormatGreeksCompact(ask_row->greeks).c_str());
    }
  }

  ImGui::EndTable();
}

void OrderBookPanel::RenderL3Ladder(const OrderBookSnapshot &snapshot) {
  uint64_t max_bid_size = MaxLevelSize(snapshot.l3_bids);
  uint64_t max_ask_size = MaxLevelSize(snapshot.l3_asks);
  uint64_t max_size = std::max(max_bid_size, max_ask_size);

  ImGui::Checkbox("Expand orders", &m_expand_l3);

  int column_count = m_show_greeks ? 8 : 6;
  ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                          ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_SizingStretchProp;

  if (!ImGui::BeginTable("L3Ladder", column_count, flags, ImVec2(0, 0)))
    return;

  if (m_show_greeks)
    ImGui::TableSetupColumn("Bid Greeks", ImGuiTableColumnFlags_WidthStretch,
                            1.6f);
  ImGui::TableSetupColumn("Bid Orders", ImGuiTableColumnFlags_WidthStretch,
                          0.7f);
  ImGui::TableSetupColumn("Bid Size", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Bid Price", ImGuiTableColumnFlags_WidthStretch,
                          1.0f);
  ImGui::TableSetupColumn("Ask Price", ImGuiTableColumnFlags_WidthStretch,
                          1.0f);
  ImGui::TableSetupColumn("Ask Size", ImGuiTableColumnFlags_WidthStretch, 1.0f);
  ImGui::TableSetupColumn("Ask Orders", ImGuiTableColumnFlags_WidthStretch,
                          0.7f);
  if (m_show_greeks)
    ImGui::TableSetupColumn("Ask Greeks", ImGuiTableColumnFlags_WidthStretch,
                            1.6f);
  ImGui::TableHeadersRow();

  int row_count = std::min<int>(
      m_visible_depth, static_cast<int>(std::max(snapshot.l3_bids.size(),
                                                 snapshot.l3_asks.size())));

  for (int row = 0; row < row_count; ++row) {
    ImGui::TableNextRow();
    bool has_bid = row < static_cast<int>(snapshot.l3_bids.size());
    bool has_ask = row < static_cast<int>(snapshot.l3_asks.size());
    bool is_best = row == 0;

    const L3Level *bid_level = has_bid ? &snapshot.l3_bids[row] : nullptr;
    const L3Level *ask_level = has_ask ? &snapshot.l3_asks[row] : nullptr;

    int col = 0;

    if (m_show_greeks) {
      ImGui::TableSetColumnIndex(col++);
      if (bid_level)
        ImGui::TextColored(
            kColorGreeksText, "%s",
            FormatGreeksCompact(bid_level->aggregate_greeks).c_str());
    }

    ImGui::TableSetColumnIndex(col++);
    if (bid_level)
      ImGui::TextColored(kColorMuted, "%zu", bid_level->orders.size());

    ImGui::TableSetColumnIndex(col++);
    if (bid_level) {
      uint64_t size = bid_level->TotalSize();
      float fraction =
          max_size > 0 ? static_cast<float>(size) / static_cast<float>(max_size)
                       : 0.0f;
      DrawSizeBar(fraction, is_best ? kColorBidBarBest : kColorBidBar, true);
      ImGui::TextColored(kColorBidText, "%llu",
                         static_cast<unsigned long long>(size));
    }

    ImGui::TableSetColumnIndex(col++);
    if (bid_level)
      ImGui::TextColored(kColorBidText, "%.4f", bid_level->price);

    ImGui::TableSetColumnIndex(col++);
    if (ask_level)
      ImGui::TextColored(kColorAskText, "%.4f", ask_level->price);

    ImGui::TableSetColumnIndex(col++);
    if (ask_level) {
      uint64_t size = ask_level->TotalSize();
      float fraction =
          max_size > 0 ? static_cast<float>(size) / static_cast<float>(max_size)
                       : 0.0f;
      DrawSizeBar(fraction, is_best ? kColorAskBarBest : kColorAskBar, false);
      ImGui::TextColored(kColorAskText, "%llu",
                         static_cast<unsigned long long>(size));
    }

    ImGui::TableSetColumnIndex(col++);
    if (ask_level)
      ImGui::TextColored(kColorMuted, "%zu", ask_level->orders.size());

    if (m_show_greeks) {
      ImGui::TableSetColumnIndex(col++);
      if (ask_level)
        ImGui::TextColored(
            kColorGreeksText, "%s",
            FormatGreeksCompact(ask_level->aggregate_greeks).c_str());
    }

    if (!m_expand_l3)
      continue;

    // Per-order breakdown, indented, muted — individual MBO entries
    // that make up this aggregated price level.
    size_t max_orders = std::max(bid_level ? bid_level->orders.size() : 0,
                                 ask_level ? ask_level->orders.size() : 0);
    for (size_t order_idx = 0; order_idx < max_orders; ++order_idx) {
      ImGui::TableNextRow();
      col = 0;

      const L3Order *bid_order =
          (bid_level && order_idx < bid_level->orders.size())
              ? &bid_level->orders[order_idx]
              : nullptr;
      const L3Order *ask_order =
          (ask_level && order_idx < ask_level->orders.size())
              ? &ask_level->orders[order_idx]
              : nullptr;

      if (m_show_greeks) {
        ImGui::TableSetColumnIndex(col++);
        if (bid_order)
          ImGui::TextColored(kColorGreeksText, "  %s",
                             FormatGreeksCompact(bid_order->greeks).c_str());
      }
      ImGui::TableSetColumnIndex(col++);
      if (bid_order)
        ImGui::TextColored(
            kColorOrderId, "#%llu",
            static_cast<unsigned long long>(bid_order->order_id));

      ImGui::TableSetColumnIndex(col++);
      if (bid_order)
        ImGui::TextColored(kColorMuted, "  %llu",
                           static_cast<unsigned long long>(bid_order->size));

      ImGui::TableSetColumnIndex(col++);
      // price intentionally blank at order granularity — shown on the level row
      // above

      ImGui::TableSetColumnIndex(col++);

      ImGui::TableSetColumnIndex(col++);
      if (ask_order)
        ImGui::TextColored(kColorMuted, "  %llu",
                           static_cast<unsigned long long>(ask_order->size));

      ImGui::TableSetColumnIndex(col++);
      if (ask_order)
        ImGui::TextColored(
            kColorOrderId, "#%llu",
            static_cast<unsigned long long>(ask_order->order_id));

      if (m_show_greeks) {
        ImGui::TableSetColumnIndex(col++);
        if (ask_order)
          ImGui::TextColored(kColorGreeksText, "  %s",
                             FormatGreeksCompact(ask_order->greeks).c_str());
      }
    }
  }

  ImGui::EndTable();
}

} // namespace ui
