#pragma once

#if __EMSCRIPTEN__
#else

#include "line_view.hpp"
#include "mmfile.hpp"

#include "order_id.hpp"
#include "parser/csv.hpp"
#include "parser/databento.hpp"
#include "parser/nasdaq.hpp"

struct CSVFile {
  using symbol_t = decltype(parser::csv::parse_line("")->symbol);
  using order_id_t = FixedSizeOrderID;
  using Price = FixedPoint<4>;

  static std::unique_ptr<MappedFile> open(const std::string &filename) {
    auto file = std::make_unique<MappedFile>(filename);
    if (!file->data())
      return nullptr;
    return file;
  }

  static auto symbol(const auto msg) { return msg.symbol; }

  static void process_file(auto &file, const auto &on_parsed) {
    LineView lines(reinterpret_cast<const char *>(file->data()), file->size());
    for (const auto &line : lines) {
      if (auto msg = parser::csv::parse_line(line))
        on_parsed(*msg);
      else
        std::cout << fmt::LineTag::Error("ERROR") << msg.error() << nl;
    }
  }

  static void update_book(auto &book, auto &msg) {
    parser::csv::process_csv_message(book, msg);
  }
};

struct NasdaqFile {
  using symbol_t = std::string;
  using order_id_t = uint64_t;
  using Price = FixedPoint<4>;

  static std::unique_ptr<MappedFile> open(const std::string &filename) {
    auto file = std::make_unique<MappedFile>(filename);
    if (!file->data())
      return nullptr;
    return file;
  }

  static auto symbol(const auto msg) { return msg.symbol; }

  static void process_file(auto &file, const auto &on_parsed) {
    size_t offset = 0;
    std::span<const uint8_t> data{file->data(), file->size()};
    while (offset < data.size()) {
      if (auto msg = parser::nasdaq::parse_message(data, offset))
        on_parsed(*msg);
      else
        std::cout << fmt::LineTag::Error("ERROR") << msg.error() << nl;
    }
  }

  static void update_book(auto &book, auto &msg) {
    parser::nasdaq::processNasdaqMessage(book, msg);
  }
};

struct DatabentoFile {
  using symbol_t = decltype(parser::csv::parse_line("")->symbol);
  using order_id_t = uint64_t;
  using Price = FixedPoint<9>;

  static auto open(const std::string &filename) {
    return parser::databento::open(filename);
  }

  static auto symbol(const auto msg) { return msg.channel_id; }

  static void process_file(auto &decoder, const auto &on_parsed) {
    while (true)
      if (auto msg = parser::databento::parse_message(*decoder)) {
        if (!*msg)
          break;
        on_parsed(**msg);
      } else
        std::cout << fmt::LineTag::Error("ERROR") << msg.error() << nl;
  }

  static void update_book(auto &book, auto &msg) {
    parser::databento::update_order_book(book, &msg);
  }
};

#endif
