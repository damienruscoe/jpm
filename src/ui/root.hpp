#pragma once

#include "ui/ui.hpp"

#include "imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <functional>
#include <memory>

namespace ui {

struct Root {
  Root();
  ~Root();

  void run();

private:
  void MainLoop();
  static void MainLoopTrampoline(void *arg);

  void process_update_queue();

  GLFWwindow *window;
  std::unique_ptr<ui::UIController> ui;
  ui::OrderBookSnapshot m_snapshot;

public:
  std::function<void(ui::OrderBookSnapshot &)> update_queue;
};

} // namespace ui
