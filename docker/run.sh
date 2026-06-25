#!/bin/bash

# docker/run.sh - Convenience wrapper for Docker-based development

PROFILE=$1
COMMAND=$2
shift 2

case "$PROFILE" in
    coverage)
        BUILD_DIR="build_coverage"
        CMAKE_FLAGS="-DENABLE_COVERAGE=ON"
        ;;
    fuzz)
        BUILD_DIR="build_fuzz"
        CMAKE_FLAGS="-DENABLE_FUZZING=ON"
        ;;
    *)
        BUILD_DIR="build"
        CMAKE_FLAGS=""
        ;;
esac

# Function to build if not already built
ensure_built() {
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Building $PROFILE profile..."
        docker compose run --rm builder bash -c "mkdir -p $BUILD_DIR && cd $BUILD_DIR && cmake $CMAKE_FLAGS .. && make"
    fi
}

case "$COMMAND" in
    up)
        ensure_built
        if [ "$PROFILE" == "coverage" ]; then
            echo "Running tests with coverage..."
            docker compose run --rm builder bash -c "cd $BUILD_DIR && ./matching_engine_tests && llvm-profdata-14 merge -sparse coverage.profraw -o coverage.profdata && llvm-cov-14 report -instr-profile=coverage.profdata ./matching_engine_tests src/ test/"
        else
            echo "Running tests..."
            docker compose run --rm builder bash -c "cd $BUILD_DIR && ./matching_engine_tests"
        fi
        ;;
    run)
        ensure_built
        # $1 is the input file path passed to run
        INPUT_FILE=$1
        echo "Executing $PROFILE build with $INPUT_FILE..."
        docker compose run --rm builder bash -c "cd $BUILD_DIR && ./matching_engine $INPUT_FILE"
        ;;
    *)
        echo "Usage: ./docker/run.sh <profile> <command> [args]"
        echo "Profiles: coverage, fuzz, default"
        echo "Commands: up, run"
        ;;
esac
