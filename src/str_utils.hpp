#pragma once

#include <utility>
#include <charconv>
#include <string_view>

std::pair<std::string_view, std::string_view> split(std::string_view s,
                                                    char delim);
std::string_view trim_inline_comments(std::string_view str);
std::string_view trim_whitespace(std::string_view str);
std::string_view trim_whitespace_prefix(std::string_view str);
std::string_view trim_whitespace_suffix(std::string_view str);

bool parse_integer(std::string_view str, auto &output) {
  auto start = str.data();
  auto end = start + str.size();
  auto [ptr, ec] = std::from_chars(start, end, output);

  const bool success = ec == std::errc{} && ptr == end;
  return success;
};
