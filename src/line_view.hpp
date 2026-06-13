#pragma once

#include <string_view>

class LineView {
public:
  class Iterator {
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::string_view;
    using difference_type = std::ptrdiff_t;
    using pointer = const std::string_view *;
    using reference = const std::string_view &;

    Iterator(const char *curr, const char *end) : curr_(curr), end_(end) {
      find_next();
    }

    reference operator*() const { return current_line_; }
    pointer operator->() const { return &current_line_; }

    Iterator &operator++() {
      find_next();
      return *this;
    }

    bool operator==(const Iterator &other) const {
      return is_end_ == other.is_end_ && (is_end_ || curr_ == other.curr_);
    }
    bool operator!=(const Iterator &other) const { return !(*this == other); }

  private:
    void find_next() {
      if (curr_ >= end_) {
        is_end_ = true;
        return;
      }

      const char *line_end = curr_;
      while (line_end < end_ && *line_end != '\n' && *line_end != '\r') {
        line_end++;
      }

      current_line_ = std::string_view(curr_, line_end - curr_);

      if (line_end < end_) {
        if (*line_end == '\r') {
          line_end++;
          if (line_end < end_ && *line_end == '\n')
            line_end++;
        } else {
          line_end++;
        }
      }
      curr_ = line_end;
    }

    const char *curr_;
    const char *end_;
    std::string_view current_line_;
    bool is_end_ = false;

    friend class LineView;
  };

  explicit LineView(const char *data, size_t size) : data_(data), size_(size) {}

  Iterator begin() const {
    if (!data_)
      return end();
    return Iterator(data_, data_ + size_);
  }

  Iterator end() const {
    Iterator it(data_ + size_, data_ + size_);
    it.is_end_ = true;
    return it;
  }

private:
  const char *data_;
  size_t size_;
};
