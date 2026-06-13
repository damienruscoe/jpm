#include "line_view.hpp"
#include "mmfile.hpp"
#include "parser.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
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
      std::cout << *msg << std::endl;
      count++;
    }
  }
  std::cout << "--- Parser Test Finished. Successfully parsed " << count
            << " messages. ---" << std::endl;

  return 0;
}
