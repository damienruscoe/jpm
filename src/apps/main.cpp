#include "fixed_point.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "order_id.hpp"
#include "parser.hpp"
#include "render.hpp"

#include <iostream>
#include <memory>

[[maybe_unused]] constexpr std::string_view VALID = "[\033[32mVALID\033[0m] ";
[[maybe_unused]] constexpr std::string_view ERROR = "[\033[31mERROR\033[0m] ";
[[maybe_unused]] constexpr std::string_view TRADE = "[\033[94mTRADE\033[0m] ";
[[maybe_unused]] constexpr std::string_view ORDER = "[\033[95mORDER\033[0m] ";

using Traits = OrderBookTraits<FixedSizeOrderID, FixedPoint<4>, uint32_t>;
using Book = OrderBook<Traits>;

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "docs/given_example.csv";

  MappedFile file(filename);
  if (!file.data()) {
    std::cerr << "Failed to open or map file: " << filename << nl;
    return 1;
  }

  LineView lines(file.data(), file.size());

  std::unordered_map<uint32_t, Book> ticker_books;

  for (const auto &line : lines) {
    if (auto msg = parse_line(line)) {
      // std::cout << VALID << *msg << nl;

      auto [it, added] = ticker_books.try_emplace(msg->exchange_ticker);
      auto &book = it->second;

      if (added) {
        book.setOnTradeCallback([](side_t side, const auto &id,
                                   const auto &price, const auto &qty,
                                   auto fill) {
          const int ticker = 101;
          const char aggressor_side = side == side_t::ASK ? 'S' : 'B';

          std::cout
              << TRADE << "Trade: " << ticker << " " << id << " " << qty << " "
              << price << ", AggrSide=" << aggressor_side
              << (fill == PriceLevel<
                              OrderBook<Traits>::Order>::FillStatus::Partial
                      ? " (Partial)"
                      : "")
              << nl;
        });
      }

      const auto side = msg->side == Side::Buy ? side_t::BID : side_t::ASK;

      switch (msg->type) {
      case RequestType::New: {
        auto added =
            book.newOrder(msg->order_id, side, msg->price, msg->quantity);
        if (!added)
          std::cout << ERROR << "Adding new order failed" << nl;
        break;
      }
      case RequestType::Cancel: {
        auto cancelled = book.cancel(msg->order_id, side);
        if (!cancelled)
          std::cout << ERROR << "Cancelling existing order failed" << nl;
        break;
      }
      case RequestType::Amend: {
        auto amended =
            book.amend(msg->order_id, side, msg->price, msg->quantity);
        if (!amended)
          std::cout << ERROR << "Amending order failed" << nl;
        break;
      }
      }

      // render_horizontal_orderbook(book);
      // render_vertical_orderbook(book);
    } else
      std::cout << ERROR << msg.error() << nl;
  }

  std::cout << nl << "<on exit>";

  for (const auto &[id, book] : ticker_books) {
    std::cout << "\n\033[30;47m Ticker: " << id << " \033[0m" << nl << nl;
    for (const auto &order : book.getOrders()) {
      const auto &order_id = order->id;
      const auto &side = order->side;
      const auto &quantity = order->quantity;
      const auto &price = order->price;

      std::cout << ORDER << "OrderId: " << order_id
                << " Side: " << (side == side_t::BID ? "Buy" : "Sell")
                << " Quantity: " << quantity << " Price: " << price << nl;
    }
    std::cout << nl;
    render_horizontal_orderbook(book);
  }

  return 0;
}
