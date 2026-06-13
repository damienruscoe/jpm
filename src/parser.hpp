#pragma once

#include "FixedPoint.hpp"
#include "mmfile.hpp"

#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>

enum class RequestType { New, Cancel, Amend, Invalid };
enum class Side { Buy, Sell, Invalid };

struct Message {
  uint32_t exchange_ticker;
  RequestType type;
  std::string_view order_id;
  Side side;
  uint32_t quantity;
  FixedPoint price;
};

std::optional<Message> parse_line(std::string_view line) {
	if (line.empty())
		return std::nullopt;

	auto split = [](std::string_view s, char delim) {
		size_t pos = s.find(delim);
		if (pos == std::string_view::npos)
			return std::pair{s, std::string_view{}};
		return std::pair{s.substr(0, pos), s.substr(pos + 1)};
	};

	auto [ticker_str, rest1] = split(line, ',');
	auto [type_str, rest2] = split(rest1, ',');
	auto [id_str, rest3] = split(rest2, ',');
	auto [side_str, rest4] = split(rest3, ',');
	auto [qty_str, price_str] = split(rest4, ',');

	if (price_str.empty() && !qty_str.empty() &&
			rest4.find(',') == std::string_view::npos) {
		// This handles the case where we might have missing columns
		// The split logic above is simple; let's refine it or check counts.
	}

	// Basic validation of column presence
	if (ticker_str.empty() || type_str.empty() || id_str.empty() ||
			side_str.empty() || qty_str.empty() || price_str.empty()) {
		std::cerr << "Error: Invalid message format (missing columns): " << line
							<< std::endl;
		return std::nullopt;
	}

	Message msg;

	// 1. Ticker
	if (auto [ptr, ec] = std::from_chars(
					ticker_str.data(), ticker_str.data() + ticker_str.size(),
					msg.exchange_ticker);
			ec != std::errc{}) {
		std::cerr << "Error: Invalid ticker: " << ticker_str << std::endl;
		return std::nullopt;
	}

	// 2. Type
	if (type_str == "N")
		msg.type = RequestType::New;
	else if (type_str == "C")
		msg.type = RequestType::Cancel;
	else if (type_str == "A")
		msg.type = RequestType::Amend;
	else {
		std::cerr << "Error: Invalid request type: " << type_str << std::endl;
		return std::nullopt;
	}

	// 3. Order ID
	if (id_str.size() > 10) {
		std::cerr << "Error: Order ID too long: " << id_str << std::endl;
		return std::nullopt;
	}
	msg.order_id = id_str;

	// 4. Side
	if (side_str == "B")
		msg.side = Side::Buy;
	else if (side_str == "S")
		msg.side = Side::Sell;
	else {
		std::cerr << "Error: Invalid side: " << side_str << std::endl;
		return std::nullopt;
	}

	// 5. Quantity
	if (auto [ptr, ec] = std::from_chars(
					qty_str.data(), qty_str.data() + qty_str.size(), msg.quantity);
			ec != std::errc{}) {
		std::cerr << "Error: Invalid quantity: " << qty_str << std::endl;
		return std::nullopt;
	}

	// 6. Price
	try {
		msg.price = FixedPoint::Parse(price_str);
	} catch (...) {
		std::cerr << "Error: Invalid price: " << price_str << std::endl;
		return std::nullopt;
	}

	return msg;
}





/**
 * @brief Lazy line iterator that iterates over a memory-mapped file.
 */
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
      while (line_end < end_ && *line_end != '\n') {
        line_end++;
      }

      current_line_ = std::string_view(curr_, line_end - curr_);
      curr_ = (line_end < end_) ? line_end + 1 : end_;
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

  bool is_ready() const { return data_ != nullptr; }

private:
  const char *data_;
  size_t size_;
};
