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

    Iterator(const char *curr, const char *end) : m_curr(curr), m_end(end) {
      find_next();
    }

    reference operator*() const { return m_current_line; }
    pointer operator->() const { return &m_current_line; }

    Iterator &operator++() {
      find_next();
      return *this;
    }

    bool operator==(const Iterator &other) const {
      return m_is_end == other.m_is_end && (m_is_end || m_curr == other.m_curr);
    }
    bool operator!=(const Iterator &other) const { return !(*this == other); }

  private:
    void find_next() {
      if (m_curr >= m_end) {
        m_is_end = true;
        return;
      }

      const char *line_end = m_curr;
      while (line_end < m_end && *line_end != '\n' && *line_end != '\r') {
        line_end++;
      }

      m_current_line = std::string_view(m_curr, line_end - m_curr);

      if (line_end < m_end) {
        if (*line_end == '\r') {
          line_end++;
          if (line_end < m_end && *line_end == '\n')
            line_end++;
        } else {
          line_end++;
        }
      }
      m_curr = line_end;
    }

    const char *m_curr;
    const char *m_end;
    std::string_view m_current_line;
    bool m_is_end = false;

    friend class LineView;
  };

  explicit LineView(const char *data, size_t size)
      : m_data(data), m_size(size) {}

  Iterator begin() const {
    if (!m_data)
      return end();
    return Iterator(m_data, m_data + m_size);
  }

  Iterator end() const {
    Iterator it(m_data + m_size, m_data + m_size);
    it.m_is_end = true;
    return it;
  }

private:
  const char *m_data;
  size_t m_size;
};
