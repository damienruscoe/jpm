#include "str_utils.hpp"

#include <iomanip>

static constexpr std::string_view WHITESPACE = " \t\r\n\v\f";

std::pair<std::string_view, std::string_view> split(std::string_view s,
                                                    char delim) {
  size_t pos = s.find(delim);
  if (pos == std::string_view::npos)
    return std::pair{s, std::string_view{}};
  return std::pair{s.substr(0, pos), s.substr(pos + 1)};
};

std::string_view trim_inline_comments(std::string_view str) {
  size_t comment_pos = str.find("//");
  if (comment_pos != std::string_view::npos)
    str = str.substr(0, comment_pos);
  return str;
}

std::string_view trim_whitespace_prefix(std::string_view str) {
  size_t first = str.find_first_not_of(WHITESPACE);
  if (first == std::string_view::npos)
    return {};
  str.remove_prefix(first);
  return str;
}

std::string_view trim_whitespace_suffix(std::string_view str) {
  size_t last = str.find_last_not_of(WHITESPACE);
  str.remove_suffix(str.size() - last - 1);
  return str;
}

std::string_view trim_whitespace(std::string_view str) {
  return trim_whitespace_suffix(trim_whitespace_prefix(str));
}
