#!/usr/bin/env bash
set -euo pipefail

# Build the Windows .exe version of RCKTMN using MinGW.
# Outputs the `rcktmn.exe` binary in the repository root.
# Uses CMake to build raylib for Windows MinGW.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

# MinGW compilers
MINGW_COMPILER=${MINGW_COMPILER:-x86_64-w64-mingw32-g++}

echo "Building Windows version with MinGW..."

if ! command -v "$MINGW_COMPILER" &> /dev/null; then
    echo "Error: MinGW compiler '$MINGW_COMPILER' not found."
    echo "Install MinGW-w64 toolchain (e.g., 'sudo dnf install mingw64-gcc mingw64-gcc-c++')"
    exit 1
fi

# Check for CMake
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: cmake not found. Install with: sudo dnf install cmake"
    exit 1
fi

# Raylib build paths (inside scripts/)
RAYLIB_DIR="$SCRIPTS_DIR/raylib_windows"
RAYLIB_SRC="$RAYLIB_DIR/raylib"
RAYLIB_BUILD="$RAYLIB_DIR/build"
RAYLIB_LIB="$RAYLIB_BUILD/raylib/libraylib.a"

# Build raylib if needed
if [ ! -f "$RAYLIB_LIB" ]; then
    echo "Building raylib for Windows with CMake..."
    mkdir -p "$RAYLIB_DIR"
    
    # Clone raylib if not present
    if [ ! -d "$RAYLIB_SRC" ]; then
        cd "$RAYLIB_DIR"
        git clone https://github.com/raysan5/raylib.git --depth 1
        cd "$ROOT_DIR"
    fi
    
    # Create build directory and configure with CMake for MinGW
    mkdir -p "$RAYLIB_BUILD"
    cd "$RAYLIB_BUILD"
    
    echo "Configuring raylib with CMake for MinGW..."
    cmake "$RAYLIB_SRC" \
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
        -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_GAMES=OFF
    
    echo "Building raylib..."
    cmake --build . -j$(nproc)
    
    cd "$ROOT_DIR"
    echo "raylib build complete."
fi

echo "Compiling RCKTMN for Windows..."
cd "$ROOT_DIR"

$MINGW_COMPILER -std=c++17 -O2 \
  -I"$RAYLIB_SRC/src" \
  rcktmn.cpp \
  src/*.cpp \
  -o rcktmn.exe \
  "$RAYLIB_LIB" \
  -lopengl32 -lgdi32 -lwinmm -lws2_32 -static-libgcc -static-libstdc++ -static

echo "Build complete: $ROOT_DIR/rcktmn.exe"
