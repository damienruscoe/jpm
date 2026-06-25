#include "fixed_point.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "parser.hpp"
#include "render.hpp"

#include <iostream>

constexpr std::string_view VALID = "[\033[32mVALID\033[0m] ";
constexpr std::string_view ERROR = "[\033[31mERROR\033[0m] ";
constexpr std::string_view TRADE = "[\033[94mTRADE\033[0m] ";

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "docs/given_example.csv";

  MappedFile file(filename);
  if (!file.data()) {
    std::cerr << "Failed to open or map file: " << filename << std::endl;
    return 1;
  }

  LineView lines(file.data(), file.size());

  OrderBook<FixedSizeOrderID, FixedPoint<4>, uint32_t> book;
  book.setOnTradeCallback([](side_t side, const auto &id, const auto &price,
                             const auto &qty, auto fill) {
    const int ticker = 101;
    const char aggressor_side = side == side_t::ASK ? 'S' : 'B';

    std::cout
        << TRADE << "Trade: " << ticker << " " << id << " " << qty << " "
        << price << ", AggrSide=" << aggressor_side
        << (fill == PriceLevel<OrderBook<FixedSizeOrderID, FixedPoint<4>,
                                         uint32_t>::Order>::FillStatus::Partial
                ? " (Partial)"
                : "")
        << std::endl;
  });

  TopOfBookRenderer top_of_book{book};

  std::cout << "--- Starting Parser Test ---" << std::endl;
  int count = 0;
  for (const auto &line : lines) {
    if (auto msg = parse_line(line)) {
      std::cout << VALID << *msg << std::endl;
      count++;

      const auto side = msg->side == Side::Buy ? side_t::BID : side_t::ASK;

      switch (msg->type) {
      case RequestType::New: {
        auto added =
            book.newOrder(msg->order_id, side, msg->price, msg->quantity);
        if (!added)
          std::cout << ERROR << "Adding new order failed" << std::endl;
        break;
      }
      case RequestType::Cancel: {
        auto cancelled = book.cancel(msg->order_id, side);
        if (!cancelled)
          std::cout << ERROR << "Cancelling existing order failed" << std::endl;
        break;
      }
      case RequestType::Amend: {
        auto amended =
            book.amend(msg->order_id, side, msg->price, msg->quantity);
        if (!amended)
          std::cout << ERROR << "Amending order failed" << std::endl;
        break;
      }
      }

      // top_of_book.render();
      render_horizontal_orderbook(book);
      // render_vertical_orderbook(book);
    } else
      std::cout << ERROR << msg.error() << std::endl;
  }
  std::cout << "--- Parser Test Finished. Successfully parsed " << count
            << " messages. ---" << std::endl;

  return 0;
}
