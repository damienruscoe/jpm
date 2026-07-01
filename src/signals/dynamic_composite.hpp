#pragma once

#include "trade_event.hpp"
#include <memory>
#include <vector>

namespace signals {
template <typename Traits> class DynamicComposite {
  struct Concept {
    virtual ~Concept() = default;

    virtual void update(const TradeEvent<Traits> &event) = 0;
    virtual void update(const OrderMatchedEvent<Traits> &event) = 0;
  };

  template <typename Signal> struct Model : Concept {
    Signal *signal;
    Model(Signal *s) : signal(s) {}

    void update(const TradeEvent<Traits> &event) override {
      handle_event(event);
    }
    void update(const OrderMatchedEvent<Traits> &event) override {
      handle_event(event);
    }

    void handle_event(const auto &event) {
      if constexpr (requires { signal->update(event); })
        signal->update(event);
    }
  };

  std::vector<std::unique_ptr<Concept>> m_signals;

public:
  template <typename T> void addSignal(T *s) { // Accepting pointer
    m_signals.push_back(std::make_unique<Model<T>>(s));
  }

  template <typename Event> void update(const Event &event) {
    for (auto &s : m_signals)
      s->update(event);
  }
};
} // namespace signals
