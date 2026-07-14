# Compiler and Flags
CXX = clang++
CXXFLAGS = -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined
DEBUG_CXXFLAGS = -std=c++20 -ggdb -O2 -Wall -Wextra -Wpedantic -Werror
GTEST_FLAGS = -lgtest -lgtest_main

# Target executables
BUILD_DIR = build
TARGET = $(BUILD_DIR)/matching_engine
UI_SAMPLE = $(BUILD_DIR)/ui_sample
TEST_TARGET = $(BUILD_DIR)/matching_engine_tests

# Source and Headers
INC = -Isrc/ -Isrc/core/ -Ideps/cnl/include
SRCS = src/apps/main.cpp src/*.cpp src/parser/*.cpp
TEST_SRCS = test/*.cpp src/*.cpp src/parser/*.cpp
UI_SRCS = src/apps/sample_dashboard.cpp src/ui/*.cpp \
	/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src/*.cpp \
	/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src/backends/imgui_impl_glfw.cpp \
	/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src/backends/imgui_impl_glut.cpp \
	/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src/backends/imgui_impl_null.cpp \
	/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src/backends/imgui_impl_opengl2.cpp \
	/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src/backends/imgui_impl_opengl3.cpp \
	/home/druscoe/dev/finance/chronovis/build.native/_deps/implot-src/*.cpp
UI_INC = \
	-I/home/druscoe/dev/finance/chronovis/build.native/_deps/glfw-build/src \
	-I/home/druscoe/dev/finance/chronovis/build.native/_deps/glfw-src \
	-I/home/druscoe/dev/finance/chronovis/build.native/_deps/glfw-subbuild \
	-I/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src \
	-I/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src/backends \
	-I/home/druscoe/dev/finance/chronovis/build.native/_deps/implot-src

#/home/druscoe/dev/finance/chronovis/build.native/_deps/imgui-src/backends/imgui_impl_glfw.h

CSV ?= docs/given_example.csv

all: test sample build/ui_sample

$(TARGET): $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INC) $(SRCS) -o $(TARGET)

$(UI_SAMPLE): $(UI_SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INC) $(UI_INC) $(UI_SRCS) -o $(UI_SAMPLE) -lGL -lglut -lglfw

debug: $(SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(DEBUG_CXXFLAGS) $(INC) $(SRCS) -o $(TARGET)_debug
	gdb --args ./$(TARGET)_debug docs/given_example.csv

$(TEST_TARGET): $(TEST_SRCS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INC) $(TEST_SRCS) -o $(TEST_TARGET) $(GTEST_FLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET) $(CSV)

sample: $(TARGET)
	./$(TARGET) docs/given_example.csv

format:
	clang-format -i test/*.cpp src/*.hpp src/*.cpp src/*/*.hpp src/*/*.cpp

clean:
	rm -fr $(BUILD_DIR)

.PHONY: all run clean run_tests debug

