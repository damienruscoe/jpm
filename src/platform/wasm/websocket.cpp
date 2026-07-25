#include "platform/wasm/websocket.hpp"

#include <string>

EM_BOOL ws::OnOpen(int eventType,
                   const EmscriptenWebSocketOpenEvent *websocketEvent,
                   void *userData) {
  std::cout << "[WASM] Connected securely via WSS!" << std::endl;
  return EM_TRUE;
}

EM_BOOL ws::OnMessage(int eventType,
                      const EmscriptenWebSocketMessageEvent *websocketEvent,
                      void *userData) {
  if (websocketEvent->isText) {
    if (userData) {
      try {
        std::cout << "[WASM] Received text: " << websocketEvent->data
                  << std::endl;
        typedef void (*MsgHandler)(const std::string &);
        const auto handler = reinterpret_cast<MsgHandler>(userData);
        std::string msg(reinterpret_cast<const char *>(websocketEvent->data));
        (handler)(msg);
      } catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
      }
    }
  }
  return EM_TRUE;
}

EM_BOOL ws::OnError(int eventType,
                    const EmscriptenWebSocketErrorEvent *websocketEvent,
                    void *userData) {
  std::cerr << "[WASM] Connection error occurred!" << std::endl;
  return EM_TRUE;
}

EM_BOOL ws::OnClose(int eventType,
                    const EmscriptenWebSocketCloseEvent *websocketEvent,
                    void *userData) {
  std::cout << "[WASM] Connection closed. Code: " << websocketEvent->code
            << std::endl;
  return EM_TRUE;
}

bool ws::run(const std::string &url) {
  EmscriptenWebSocketCreateAttributes attr = {
      url.c_str(),
      nullptr, // Protocols (NULL for default)
      EM_TRUE  // Create synchronously
  };

  EMSCRIPTEN_WEBSOCKET_T ws = emscripten_websocket_new(&attr);
  if (ws <= 0) {
    std::cerr << "Failed to create WebSocket instance." << std::endl;
    return false;
  }

  emscripten_websocket_set_onopen_callback(ws, nullptr, ws::OnOpen);
  emscripten_websocket_set_onmessage_callback(ws, nullptr, ws::OnMessage);
  emscripten_websocket_set_onerror_callback(ws, nullptr, ws::OnError);
  emscripten_websocket_set_onclose_callback(ws, nullptr, ws::OnClose);

  emscripten_exit_with_live_runtime();
  return true;
}
