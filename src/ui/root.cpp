#include "ui/root.hpp"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

namespace {

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
const char *glsl_version = "#version 300 es";
void run_main_loop(const auto &MainLoop, const auto &window) {
  emscripten_set_main_loop(MainLoop, 0, 1);
}
#else
const char *glsl_version = "#version 130";
void run_main_loop(const auto &MainLoop, const auto &window) {
  while (!glfwWindowShouldClose(window)) {
    MainLoop();
  }
}
#endif

} // namespace

namespace ui {

Root::Root() {
  if (!glfwInit())
    throw std::runtime_error("Failed to init glfw");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

  window = glfwCreateWindow(1280, 720, "Instrument Orders", nullptr, nullptr);
  if (!window)
    throw std::runtime_error("Failed to init GUI window");

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ui = std::make_unique<UIController>();
}

Root::~Root() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Root::run() {
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(MainLoop, 0, 1);
#else
  while (!glfwWindowShouldClose(window))
    MainLoop();
#endif
}

void Root::MainLoop() {
  process_update_queue();

  glfwPollEvents();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ui->Render(m_snapshot);

  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  glfwSwapBuffers(window);
}

void Root::process_update_queue() { update_queue(m_snapshot); }

} // namespace ui
