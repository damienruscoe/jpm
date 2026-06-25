# Compiler and Flags
CXX = clang++
CXXFLAGS = -std=c++20 -ggdb -O3 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
GTEST_FLAGS = -lgtest -lgtest_main

# Target executables
BUILD_DIR = build
TARGET = $(BUILD_DIR)/matching_engine
TEST_TARGET = $(BUILD_DIR)/matching_engine_tests

# Source and Headers
INC = -Isrc/ -Ideps/cnl/include
SRCS = src/apps/main.cpp src/*.cpp
TEST_SRCS = test/*.cpp src/*.cpp

all: test $(TARGET) sample

$(TARGET): $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INC) $(SRCS) -o $(TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INC) $(TEST_SRCS) -o $(TEST_TARGET) $(GTEST_FLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

sample: $(TARGET)
	./$(TARGET) docs/given_example.csv

format:
	clang-format -i test/*.cpp src/*.hpp src/*.cpp src/apps/*.cpp

clean:
	rm -fr $(BUILD_DIR)

.PHONY: all run clean run_tests

