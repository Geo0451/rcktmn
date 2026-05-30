#!/usr/bin/env bash
set -euo pipefail

# Build the native desktop version of RCKTMN.
# Outputs the `rcktmn` binary in the repository root.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

echo "Building desktop version..."
g++ -std=c++17 -O2 \
  rcktmn.cpp \
  src/*.cpp \
  -o rcktmn \
  -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

echo "Build complete: $ROOT_DIR/rcktmn"
