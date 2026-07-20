#include "ui/root.hpp"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

namespace {

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
const char *glsl_version = "#version 100";
#else
const char *glsl_version = "#version 130";
#endif

} // namespace

#ifndef __EMSCRIPTEN__
#include <iostream>
#endif

namespace log {

void debug(const char *msg) {
#ifdef __EMSCRIPTEN__
  EM_ASM({ console.log("[C++ LOG] " + UTF8ToString($0)); }, msg);
#else
  std::cout << "[C++ LOG] " << msg << std::endl;
#endif
}

void error(const char *msg) {
#ifdef __EMSCRIPTEN__
  EM_ASM({ console.error("[C++ ERR] " + UTF8ToString($0)); }, msg);
#else
  std::cerr << "[C++ ERR] " << msg << std::endl;
#endif
}

} // namespace log

namespace ui {

void glfw_error_callback(int error_code, const char *description) {
  log::error(description);
}

Root::Root() {
  log::debug("Entering Root::Root() constructor...");

  glfwSetErrorCallback(glfw_error_callback);
  log::debug("GLFW error callback registered.");

  log::debug("Calling glfwInit()...");
  if (!glfwInit()) {
    log::error("glfwInit() RETURNED FALSE!");
    throw std::runtime_error("Failed to init glfw");
  }

  log::debug("glfwInit() succeeded.");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  window = glfwCreateWindow(1280, 720, "Dashboard", nullptr, nullptr);

  if (!window) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_ALPHA_BITS, 0);   // Force 24-bit RGB (no alpha buffer)
    glfwWindowHint(GLFW_STENCIL_BITS, 0); // Disable stencil if unused
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    log::debug("Attempting glfwCreateWindow(1280, 720)...");
    window = glfwCreateWindow(1280, 720, "Dashboard", NULL, NULL);
  }

  if (!window) {
    log::error("glfwCreateWindow returned NULL!");
    throw std::runtime_error("Failed to init GUI window");
  }

  glfwMakeContextCurrent(window);

#ifdef __EMSCRIPTEN__
  /*
EmscriptenWebGLContextAttributes attr;
emscripten_webgl_init_context_attributes(&attr);
attr.alpha = EM_FALSE;
attr.depth = EM_FALSE;
attr.stencil = EM_FALSE;
attr.antialias = EM_FALSE;
attr.majorVersion = 1;
attr.minorVersion = 0;
attr.powerPreference = EM_WEBGL_POWER_PREFERENCE_LOW_POWER;

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx =
emscripten_webgl_create_context("#canvas", &attr);
emscripten_webgl_make_context_current(ctx);
  */
#endif // __EMSCRIPTEN__

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
  emscripten_set_main_loop_arg(MainLoopTrampoline, this, 0, 1);
  glfwSwapInterval(1);
#else
  glfwSwapInterval(1);
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

void Root::MainLoopTrampoline(void *arg) {
  static_cast<Root *>(arg)->MainLoop();
}

void Root::process_update_queue() { update_queue(m_snapshot); }

} // namespace ui
