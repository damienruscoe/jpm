# Compiler and Flags
CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror
GTEST_FLAGS = -lgtest -lgtest_main -pthread

# Target executable
TARGET = matching_engine_test
TEST_TARGET = parser_tests

# Source and Headers
SRCS = src/*.cpp
TEST_SRCS = test/*.cpp src/mmfile.cpp src/parser.cpp

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_TARGET) $(GTEST_FLAGS)

run_tests: $(TEST_TARGET)
	./$(TEST_TARGET)

format:
	clang-format -i src/*.hpp src/*.cpp

clean:
	rm -f $(TARGET) $(TEST_TARGET)

.PHONY: all run clean run_tests
