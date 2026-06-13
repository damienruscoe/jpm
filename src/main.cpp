#include "parser.hpp"
#include <iomanip>
#include <iostream>

void print_message(const Message &msg) {
  std::cout << "Ticker: " << msg.exchange_ticker << " | Type: "
            << (msg.type == RequestType::New      ? "N"
                : msg.type == RequestType::Cancel ? "C"
                                                  : "A")
            << " | ID: " << std::setw(10) << msg.order_id
            << " | Side: " << (msg.side == Side::Buy ? "B" : "S")
            << " | Qty: " << msg.quantity << " | Price: " << std::fixed
            << std::setprecision(2) << msg.price.ToDouble() << std::endl;
}

int main() {
  const std::string filename = "test/test_data.csv";
  Parser parser(filename);

  if (!parser.is_ready()) {
    std::cerr << "Failed to open or map file: " << filename << std::endl;
    return 1;
  }

  std::cout << "--- Starting Parser Test ---" << std::endl;
  int count = 0;
  for (const auto &msg : parser) {
    print_message(msg);
    count++;
  }
  std::cout << "--- Parser Test Finished. Successfully parsed " << count
            << " messages. ---" << std::endl;

  return 0;
}
