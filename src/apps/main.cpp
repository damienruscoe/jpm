#include "order_book.hpp"
#include "parser/parsers.hpp"
#include "render.hpp"

#include <iostream>

std::string to_buy_sell_str(side_t side) {
  return side == side_t::BID ? "Buy" : "Sell";
}

char to_buy_sell_ch(side_t side) { return to_buy_sell_str(side)[0]; }

template <typename Traits> struct TradePrinter {
  void update(const TradeEvent<Traits> &event) {
    std::cout << fmt::LineTag::Note("TRADED")
              << fmt::KeyValue("Order ID", event.order_id)
              << fmt::KeyValue("Quantity", event.quantity)
              << fmt::KeyValue("Price", event.price)
              << fmt::KeyValue("Aggressive Side",
                               to_buy_sell_ch(event.aggressor_side));
    if (event.fill == FillStatus::Partial)
      std::cout << fmt::LineTag::Note("Partial");
    std::cout << nl;
  }

  void update(const OrderMatchedEvent<Traits> &event) {
    if (event.remaining == 0)
      std::cout << fmt::LineTag::Warning("FILLED");
    else
      std::cout << fmt::LineTag::Debug("RESTED");

    std::cout << fmt::KeyValue("Order ID", event.order_id)
              << fmt::KeyValue("Quantity", event.quantity)
              << fmt::KeyValue("Price", event.price)
              << fmt::KeyValue("Side", to_buy_sell_ch(event.side)) << nl;
  }

  void update(const LevelQuantityEvent<Traits> &event) { (void)event; }
};

using FileSource = CSVFile;
// using FileSource = DatabentoFile;
// using FileSource = NasdaqFile;

using Traits =
    OrderBookTraits<FileSource::order_id_t, FileSource::Price, uint32_t>;

using Book = OrderBook<Traits, TradePrinter<Traits>>;

int main(int argc, char *argv[]) {
  std::string filename = argc > 1 ? argv[1] : "../docs/given_example.csv";

  auto file = FileSource::open(filename);
  if (!file) {
    std::cerr << "Failed to open or map file: " << filename << nl;
    return 1;
  }

  std::unordered_map<FileSource::symbol_t, Book> ticker_books;
  FileSource::process_file(file, [&](const auto &msg) {
    std::cout << fmt::LineTag::Message("VALID") << msg << nl;

    auto [it, added] = ticker_books.try_emplace(FileSource::symbol(msg));
    auto &book = it->second;

    FileSource::update_book(book, msg);
    render_horizontal_orderbook(book);
  });

  std::cout << nl << "<on exit>";

  for (const auto &[symbol, book] : ticker_books) {
    std::cout << "\n\033[30;47m Symbol: " << symbol << " \033[0m" << nl << nl;
    for (const auto &order : book.getOrders()) {
      std::cout << fmt::LineTag::Warning("ORDER")
                << fmt::KeyValue("OrderId", order->id)
                << fmt::KeyValue("Side", to_buy_sell_str(order->side))
                << fmt::KeyValue("Quantity", order->quantity)
                << fmt::KeyValue("Price", order->price) << nl;
    }
    std::cout << nl;
    render_horizontal_orderbook(book);
  }
}
