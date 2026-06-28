#pragma once
#include "trade_event.hpp"

namespace signals {
template <typename Traits, typename... Signals>
class StaticComposite : public Signals... {
public:
  void update(const TradeEvent<Traits> &event) {
    (Signals::update(event), ...);
  }
};
} // namespace signals
