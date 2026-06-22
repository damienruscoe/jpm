# Compiler and Flags
CXX = clang++
CXXFLAGS = -std=c++20 -ggdb -O3 -Wall -Wextra -Wpedantic -Werror

ifdef USE_BOOST
CXXFLAGS += -DUSE_BOOST_FP
endif

GTEST_FLAGS = -lgtest -lgtest_main -pthread

# Target executable
TARGET = matching_engine
TEST_TARGET = matching_engine_tests

# Source and Headers
INC = -Isrc/ -Ideps/cnl/include
SRCS = src/apps/main.cpp src/*.cpp
FUZZ_SRCS = src/apps/fuzz_harness.cpp src/mmfile.cpp src/parser.cpp
TEST_SRCS = test/*.cpp src/mmfile.cpp src/parser.cpp

# AFL target
all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INC) $(SRCS) -o $(TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(INC) $(TEST_SRCS) -o $(TEST_TARGET) $(GTEST_FLAGS)

run_tests: $(TEST_TARGET)
	./$(TEST_TARGET)

format:
	clang-format -i test/*.cpp src/*.hpp src/*.cpp src/apps/*.cpp

clean:
	rm -f $(TARGET) $(TEST_TARGET) fuzz_harness

fuzz_build:
	afl-clang-fast++ $(CXXFLAGS) $(INC) $(FUZZ_SRCS) -o fuzz_harness

fuzz:
	AFL_SKIP_CPUFREQ=1 afl-fuzz -i fuzz_in -o fuzz_out ./fuzz_harness

.PHONY: all run clean run_tests afl_fuzz

