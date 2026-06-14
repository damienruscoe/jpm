#include "parser.hpp"
#include "line_view.hpp"
#include <iostream>
#include <vector>
#include <iterator>

// AFL++ harness
int main() {
    // Read input from stdin
    std::vector<char> buffer((std::istreambuf_iterator<char>(std::cin)),
                              std::istreambuf_iterator<char>());

    if (buffer.empty()) return 0;

    // Use a LineView or feed the buffer directly to parse_line as needed
    // Given the parser seems to work line-by-line:
    LineView lines(buffer.data(), buffer.size());
    for (const auto &line : lines) {
        parse_line(line);
    }

    return 0;
}
