#pragma once

#include <emscripten.h>
#include <emscripten/websocket.h>
#include <iostream>

namespace ws {

EM_BOOL OnOpen(int eventType,
               const EmscriptenWebSocketOpenEvent *websocketEvent,
               void *userData);
EM_BOOL OnMessage(int eventType,
                  const EmscriptenWebSocketMessageEvent *websocketEvent,
                  void *userData);
EM_BOOL OnError(int eventType,
                const EmscriptenWebSocketErrorEvent *websocketEvent,
                void *userData);
EM_BOOL OnClose(int eventType,
                const EmscriptenWebSocketCloseEvent *websocketEvent,
                void *userData);

bool run(const std::string &url);

void connect(const std::string &url, const auto &on_parsed) {
  EmscriptenWebSocketCreateAttributes attr = {
      url.c_str(),
      nullptr, // Protocols (NULL for default)
      EM_TRUE  // Create synchronously
  };

  EMSCRIPTEN_WEBSOCKET_T ws = emscripten_websocket_new(&attr);
  if (ws <= 0) {
    std::cerr << "Failed to create WebSocket instance." << std::endl;
    return;
  }

  emscripten_websocket_set_onopen_callback(ws, nullptr, ws::OnOpen);
  emscripten_websocket_set_onmessage_callback(ws, (void *)&on_parsed,
                                              ws::OnMessage);
  emscripten_websocket_set_onerror_callback(ws, nullptr, ws::OnError);
  emscripten_websocket_set_onclose_callback(ws, nullptr, ws::OnClose);

  emscripten_exit_with_live_runtime();
  return;
}

} // namespace ws
