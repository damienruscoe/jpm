#include "ui/ui.hpp"
#include "ui/dashboard.hpp"
#include "ui/dashboard_types.hpp"

#include "imgui.h"
#include "implot.h"
#include "implot_internal.h"

#include <vector>

namespace {

void UpdateTheme() {
  ImPlot::PushColormap(ImPlotColormap_Cool);
  ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0.02f, 0.02f, 0.05f, 1.00f));
  ImPlot::PushStyleColor(ImPlotCol_FrameBg,
                         ImVec4(0.105f, 0.05f, 0.08f, 1.00f));
  ImPlot::PushStyleColor(ImPlotCol_PlotBorder,
                         ImVec4(0.00f, 0.80f, 1.00f, 0.50f));
}

void PopTheme() {
  ImPlot::PopStyleColor(3);
  ImPlot::PopColormap();
}

} // namespace

namespace ui {

UIController::UIController() {}

void UIController::Render(ui::OrderBookSnapshot &snapshot) {
  ImGui::StyleColorsDark();

  ImVec2 viewportSize = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetNextWindowSize(viewportSize, ImGuiCond_Always);

  ImGui::Begin("Instrument Orders", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

  UpdateTheme();

  if (ImGui::BeginTabBar("Streams")) {
    if (ImGui::BeginTabItem("Level Order Book")) {
      m_obp.Render(snapshot);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  PopTheme();

  ImGui::End();
}

} // namespace ui
