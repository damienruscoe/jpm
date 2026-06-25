# Makefile for jpmorgan.gitrestructure
.PHONY: image run build test fuzz

# Default CSV file if none provided
CSV ?= ./docs/given_example.csv

test: image
	docker run --rm -v "$(shell pwd):/workspace" jpmorgan-submission bash -c "\
		mkdir -p /workspace/build && cd /workspace/build && \
		cmake -DENABLE_COVERAGE=ON .. && make -j8 coverage matching_engine"

image:
	docker build -t jpmorgan-submission -f docker/Dockerfile .

build: image
	docker run --rm -v "$(shell pwd):/workspace" jpmorgan-submission bash -c "\
		mkdir -p /workspace/build && cd /workspace/build && \
		cmake .. && make -j8 matching_engine"

# Run the app: Usage: make run CSV=./path/to/data.csv
run: build
	docker run --rm -v "$(shell pwd):/workspace" \
	-v "$(shell dirname $(realpath $(CSV))):/input_dir" \
	jpmorgan-submission /workspace/build/matching_engine /input_dir/$(shell basename $(CSV))

# Optional Fuzzing
fuzz: image
	docker run --rm -v "$(shell pwd):/workspace" jpmorgan-submission bash -c "\
		mkdir -p /workspace/build_fuzz && cd /workspace/build_fuzz && \
		cmake -DENABLE_FUZZING=ON .. && make -j8 && \
		./matching_engine /workspace/docs/given_example.csv"
