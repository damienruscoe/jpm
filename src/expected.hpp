#pragma once
#include <variant>

template <typename T, typename E> struct Expected {
  std::variant<T, E> data;

  Expected(T v) : data(std::move(v)) {}
  Expected(E e) : data(std::move(e)) {}

  bool has_value() const { return std::holds_alternative<T>(data); }
  const T &value() const { return std::get<T>(data); }
  const E &error() const { return std::get<E>(data); }

  const T &operator*() const { return value(); }
  const T *operator->() const { return &value(); }

  operator bool() const { return has_value(); }
};
