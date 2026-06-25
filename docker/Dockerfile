FROM ubuntu:22.04

# Avoid tzdata interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install essential build tools and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    clang \
    llvm \
    lld \
    libgtest-dev \
    libboost-all-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
