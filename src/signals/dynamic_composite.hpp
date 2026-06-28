#pragma once

#include "trade_event.hpp"
#include <memory>
#include <vector>

namespace signals {
template <typename Traits> class DynamicComposite {
  struct Concept {
    virtual ~Concept() = default;
    virtual void update(const TradeEvent<Traits> &event) = 0;
  };

  template <typename T> struct Model : Concept {
    T *signal; // Storing pointer
    Model(T *s) : signal(s) {}
    void update(const TradeEvent<Traits> &event) override {
      signal->update(event); // Using pointer
    }
  };

  std::vector<std::unique_ptr<Concept>> m_signals;

public:
  template <typename T> void addSignal(T *s) { // Accepting pointer
    m_signals.push_back(std::make_unique<Model<T>>(s));
  }

  void update(const TradeEvent<Traits> &event) {
    for (auto &s : m_signals)
      s->update(event);
  }
};
} // namespace signals
