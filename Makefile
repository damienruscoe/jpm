CXX = g++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror

TARGET = matching_engine_test
SRCS = src/*.cpp
HDRS = src/*.hpp

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

format:
	clang-format -i src/*.hpp src/*.cpp

clean:
	rm -f $(TARGET)

.PHONY: all run clean
