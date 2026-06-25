cmake_minimum_required(VERSION 3.15)
project(MatchingEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add dependencies
add_subdirectory(deps/cnl)

# Include directories
include_directories(src/ deps/cnl/include)

# Common Compiler Flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Define source files
file(GLOB_RECURSE SRC_FILES "src/*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "src/apps/.*")

# Application Target
add_executable(matching_engine src/apps/main.cpp ${SRC_FILES})
target_compile_options(matching_engine PRIVATE -O3)

# Test Target
file(GLOB_RECURSE TEST_FILES "test/*.cpp")
add_executable(matching_engine_tests ${TEST_FILES} ${SRC_FILES})
target_compile_options(matching_engine_tests PRIVATE -O3)

# Features
option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
option(ENABLE_FUZZING "Enable fuzzing instrumentation" OFF)

if(ENABLE_COVERAGE)
    set(CMAKE_CXX_COMPILER "clang++")
    set(CMAKE_C_COMPILER "clang")
    
    target_compile_options(matching_engine_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(matching_engine_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)

    add_custom_target(coverage
        COMMAND LLVM_PROFILE_FILE=coverage.profraw ./matching_engine_tests --gtest_color=yes
        COMMAND llvm-profdata-14 merge -sparse coverage.profraw -o coverage.profdata
        COMMAND llvm-cov-14 report -use-color -instr-profile=coverage.profdata ./matching_engine_tests ../src/ ../test/
        COMMAND llvm-cov-14 show ./matching_engine_tests -instr-profile=coverage.profdata -format=html ../src/ ../test/ > coverage_report.html
        COMMAND llvm-cov-14 show ./matching_engine_tests -instr-profile=coverage.profdata -format=text ../src/ ../test/ > coverage_report.txt
        DEPENDS matching_engine_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report..."
    )
endif()


# Find GTest
find_package(GTest REQUIRED)
target_link_libraries(matching_engine_tests GTest::gtest_main GTest::gtest)
