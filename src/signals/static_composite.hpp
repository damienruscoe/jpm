#pragma once
#include "trade_event.hpp"
#include <type_traits>

namespace signals {
template <typename... Signals> class StaticComposite : public Signals... {
public:
  template <typename Event> void update(const Event &event) {
    (handle_event(static_cast<Signals &>(*this), event), ...);
  }

  void handle_event(auto &signal, const auto &event) {
    if constexpr (requires { signal.update(event); })
      signal.update(event);
  }
};
} // namespace signals
