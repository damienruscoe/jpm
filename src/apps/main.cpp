#include "FixedPoint.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "parser.hpp"
#include "price_time_matching_engine.hpp"
#include "render.hpp"

#include <iostream>

struct BtcUsd {
  using Price = FixedPointAI;
  using Quantity = uint32_t;

  // using Price = FixedPointAI<2, uint64_t>;
  //  using Price = FixedPointAI<3, uint64_t>;
  // using Quantity = FixedPointAI<8, uint32_t>;
  //  using Quantity = FixedPointAI<8, uint64_t>;

  friend std::ostream &operator<<(std::ostream &os, const BtcUsd &level) {
    return os << level.price << ' ' << level.quantity;
  }

  // auto operator<=>(const BtcUsd& level) const = default;

  Price price{};
  Quantity quantity{};
};

using Book = OrderBook<BtcUsd, PriceTimeMatchingEngine>;
Book book;
TopOfBookRender<Book> top_of_book;

int main(int argc, char *argv[]) {
  std::string filename = "test/test_data.csv";
  if (argc > 1) {
    filename = argv[1];
  }

  MappedFile file(filename);

  if (!file.data()) {
    std::cerr << "Failed to open or map file: " << filename << std::endl;
    return 1;
  }

  LineView lines(file.data(), file.size());

  std::cout << "--- Starting Parser Test ---" << std::endl;
  int count = 0;
  for (const auto &line : lines) {
    if (auto msg = parse_line(line)) {
      // std::cout << "[\033[32mVALID\033[0m] " << *msg << std::endl;
      //  std::cout << "[VALID] " << *msg << std::endl;
      count++;

      const auto order_id = std::string{msg->order_id};
      const auto side = msg->side == Side::Buy ? side_t::BID : side_t::ASK;

      const BtcUsd level = BtcUsd{msg->price, msg->quantity};
      switch (msg->type) {
      case RequestType::New: {
        auto added = book.update(order_id, side, level);
        std::cout << "Order " << (added ? "Added" : "Adding failed");
        break;
      }
      case RequestType::Cancel: {
        auto cancelled = book.cancel(order_id, side);
        std::cout << "Order " << (cancelled ? "Cancelled" : "Cancel failed");
        break;
      }
      case RequestType::Amend: {
        auto amended = book.amend(order_id, side, level);
        std::cout << "Order " << (amended ? "Amended" : "Amend failed");
        break;
      }
      }
      std::cout << std::endl;
      // top_of_book.render(book);
      render_book(book);
    }
  }
  std::cout << "--- Parser Test Finished. Successfully parsed " << count
            << " messages. ---" << std::endl;

  return 0;
}
