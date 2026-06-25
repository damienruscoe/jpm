#!/bin/bash

set -e

CSV_PATH=$1

if [ -z "$CSV_PATH" ]; then
    echo "Usage: ./run.sh <path_to_csv>"
    exit 1
fi

# Ensure Docker image is built
docker build -t jpmorgan-submission -f docker/Dockerfile .

# Convert relative CSV path to absolute for volume mapping
ABS_CSV_PATH=$(realpath "$CSV_PATH")

# Run build, test, coverage, and app
docker run --rm -v "$(pwd):/workspace" -v "$(dirname "$ABS_CSV_PATH"):/input_dir" jpmorgan-submission bash -c "
    if [ ! -d /workspace/build ]; then
        mkdir -p /workspace/build
        cd /workspace/build
        cmake -DENABLE_COVERAGE=ON ..
        make -j8 coverage matching_engine
    fi
    
    echo '--- Running application with: $CSV_PATH ---'
    /workspace/build/matching_engine /input_dir/$(basename "$CSV_PATH")
"
