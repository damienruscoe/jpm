#pragma once

#include <charconv>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>

std::ostream &nl(std::ostream &os);

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

namespace fmt {

namespace impl {

constexpr std::string_view BLACK{"\033[30m"};
constexpr std::string_view RED{"\033[31m"};
constexpr std::string_view GREEN{"\033[32m"};
constexpr std::string_view YELLOW{"\033[33m"};
constexpr std::string_view BLUE{"\033[34m"};
constexpr std::string_view MAGENTA{"\033[35m"};
constexpr std::string_view CYAN{"\033[36m"};
constexpr std::string_view WHITE{"\033[37m"};

constexpr std::string_view BOLD_BLACK{"\033[30;1m"};
constexpr std::string_view BOLD_RED{"\033[31;1m"};
constexpr std::string_view BOLD_GREEN{"\033[32;1m"};
constexpr std::string_view BOLD_YELLOW{"\033[33;1m"};
constexpr std::string_view BOLD_BLUE{"\033[34;1m"};
constexpr std::string_view BOLD_MAGENTA{"\033[35;1m"};
constexpr std::string_view BOLD_CYAN{"\033[36;1m"};
constexpr std::string_view BOLD_WHITE{"\033[37;1m"};

constexpr std::string_view LIGHT_GREY{"\033[90m"};
constexpr std::string_view LIGHT_RED{"\033[91m"};
constexpr std::string_view LIGHT_GREEN{"\033[92m"};
constexpr std::string_view LIGHT_YELLOW{"\033[93m"};
constexpr std::string_view LIGHT_BLUE{"\033[94m"};
constexpr std::string_view LIGHT_MAGENTA{"\033[95m"};
constexpr std::string_view LIGHT_CYAN{"\033[96m"};
constexpr std::string_view LIGHT_WHITE{"\033[97m"};

template <typename Tag> struct LineTag {
  LineTag(const Tag &tag, std::string_view colour) : tag{tag}, colour{colour} {}

  LineTag(const LineTag &) = delete;
  LineTag(LineTag &&) noexcept = delete;
  LineTag &operator=(const LineTag &) = delete;
  LineTag &operator=(LineTag &&) = delete;

  friend std::ostream &operator<<(std::ostream &os, LineTag &&line_tag) {
    return os << "[ " << line_tag.colour << std::left << std::setfill(' ')
              << std::setw(6) << line_tag.tag << "\033[0m ]  ";
  }

private:
  const Tag &tag;
  const std::string_view colour;
};

template <typename Tag>
LineTag(const Tag &, std::string_view colour) -> LineTag<Tag>;

} // namespace impl

namespace LineTag {
template <typename Tag> auto Debug(const Tag &tag) -> fmt::impl::LineTag<Tag> {
  return fmt::impl::LineTag(tag, fmt::impl::CYAN);
}

template <typename Tag> auto Info(const Tag &tag) -> fmt::impl::LineTag<Tag> {
  return fmt::impl::LineTag(tag, fmt::impl::LIGHT_GREY);
}

template <typename Tag>
auto Message(const Tag &tag) -> fmt::impl::LineTag<Tag> {
  return fmt::impl::LineTag(tag, fmt::impl::GREEN);
}

template <typename Tag>
auto Warning(const Tag &tag) -> fmt::impl::LineTag<Tag> {
  return fmt::impl::LineTag(tag, fmt::impl::YELLOW);
}

template <typename Tag> auto Error(const Tag &tag) -> fmt::impl::LineTag<Tag> {
  return fmt::impl::LineTag(tag, fmt::impl::RED);
}

template <typename Tag> auto Note(const Tag &tag) -> fmt::impl::LineTag<Tag> {
  return fmt::impl::LineTag(tag, fmt::impl::BLUE);
}
} // namespace LineTag

template <typename Key, typename Value> struct KeyValue {
  KeyValue(const Key &key, const Value &value) : key{key}, value{value} {}

  KeyValue(const KeyValue &) = delete;
  KeyValue(KeyValue &&) noexcept = delete;
  KeyValue &operator=(const KeyValue &) = delete;
  KeyValue &operator=(KeyValue &&) = delete;

  friend std::ostream &operator<<(std::ostream &os, KeyValue &&kv) {
    return os << "\033[40;1m" << kv.key << "\033[0m" << ": " << "\033[37m"
              << std::move(kv.value) << "\033[0m" << "\t";
  }

private:
  const Key &key;
  const Value &value;
};

template <typename Key, typename Value>
KeyValue(const Key &, const Value &value,
         std::string_view) -> KeyValue<Key, Value>;

template <typename T, bool Owned> struct ValueOr {
  using OptBindingRef =
      std::conditional_t<Owned, std::optional<T>, const std::optional<T> &>;

  ValueOr(OptBindingRef opt, std::string_view alt) : opt{opt}, alt{alt} {}

  ValueOr(const ValueOr &) = delete;
  ValueOr(ValueOr &&) noexcept = delete;
  ValueOr &operator=(const ValueOr &) = delete;
  ValueOr &operator=(ValueOr &&) = delete;

  friend std::ostream &operator<<(std::ostream &os, const ValueOr &&o) {
    return o.opt ? (os << *o.opt) : (os << o.alt);
  }

private:
  OptBindingRef opt;
  std::string_view alt;
};

template <typename T>
ValueOr(const std::optional<T> &, std::string_view) -> ValueOr<T, false>;

template <typename T>
ValueOr(std::optional<T> &&, std::string_view) -> ValueOr<T, true>;

} // namespace fmt
