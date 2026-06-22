#include "FixedPointGeneric.hpp"
#include "line_view.hpp"
#include "mmfile.hpp"
#include "order_book.hpp"
#include "parser.hpp"
#include "render.hpp"

#include <iostream>

struct BtcUsd {
  using OrderID = std::string;
  using Price = FixedPointGeneric;
  using Quantity = uint32_t;

  // using Price = FixedPointGeneric<2, uint64_t>;
  //  using Price = FixedPointGeneric<3, uint64_t>;
  // using Quantity = FixedPointGeneric<8, uint32_t>;
  //  using Quantity = FixedPointGeneric<8, uint64_t>;

  friend std::ostream &operator<<(std::ostream &os, const BtcUsd &level) {
    return os << level.price << ' ' << level.quantity;
  }

  bool operator<(const BtcUsd &other) const {
    if (price != other.price) return price < other.price;
    return quantity < other.quantity;
  }
  bool operator==(const BtcUsd &other) const {
    return price == other.price && quantity == other.quantity;
  }

  Price price{};
  Quantity quantity{};
};

OrderBook<BtcUsd> book;
TopOfBookRenderer top_of_book{book};

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
      std::cout << "[\033[32mVALID\033[0m] " << *msg << std::endl;
      count++;

      const auto order_id = msg->order_id;
      const auto side = msg->side == Side::Buy ? side_t::BID : side_t::ASK;

      switch (msg->type) {
      case RequestType::New: {
        auto added = book.newOrder(order_id, side, msg->price, msg->quantity);
        if (!added)
          std::cout << "[\033[31mERROR\033[0m] Adding new order failed"
                    << std::endl;
        break;
      }
      case RequestType::Cancel: {
        auto cancelled = book.cancel(order_id, side);
        if (!cancelled)
          std::cout << "[\033[31mERROR\033[0m] Cancelling existing order failed"
                    << std::endl;
        break;
      }
      case RequestType::Amend: {
        auto amended = book.amend(order_id, side, msg->price, msg->quantity);
        if (!amended)
          std::cout << "[\033[31mERROR\033[0m] Amending order failed"
                    << std::endl;
        break;
      }
      }

      // top_of_book.render();
      render_book(book);
    }
  }
  std::cout << "--- Parser Test Finished. Successfully parsed " << count
            << " messages. ---" << std::endl;

  return 0;
}
