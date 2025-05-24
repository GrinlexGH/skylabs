#!/bin/bash
set -euo pipefail

# Author: Grinlex

ROOT="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="$ROOT/bin/unix"
SRC_DIR="$ROOT/sources"

check_git_hash_match() {
    local SOURCE_DIR="$1"
    local HASH_FILE="$2"

    local GIT_HASH
    GIT_HASH=$(git -C "$SOURCE_DIR" rev-parse HEAD)

    if [ -f "$HASH_FILE" ]; then
        local EXISTING_HASH
        EXISTING_HASH=$(<"$HASH_FILE")
        if [ "$EXISTING_HASH" = "$GIT_HASH" ]; then
            return 0
        fi
    fi

    return 1
}

build_library() {
    local LIB_NAME="$1"
    local SOURCE_DIR="$2"
    local INSTALL_DIR="$3"
    local EXTRA_CMAKE_FLAGS=("${@:4}")
    local CONFIGS=("Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    local BUILD_DIR="$SOURCE_DIR/build"
    local HASH_FILE="$INSTALL_DIR/git_hash.txt"

    if [ -d "$INSTALL_DIR" ] && check_git_hash_match "$SOURCE_DIR" "$HASH_FILE"; then
        echo "[$LIB_NAME] is up to date."
        return
    fi

    echo "Compiling [$LIB_NAME]..."

    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"

    rm -rf "$INSTALL_DIR"
    mkdir -p "$INSTALL_DIR"

    for CONFIG in "${CONFIGS[@]}"; do
        cmake -G Ninja \
            -DCMAKE_BUILD_TYPE="$CONFIG" \
            -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
            "${EXTRA_CMAKE_FLAGS[@]}" \
            -S "$SOURCE_DIR" \
            -B "$BUILD_DIR"

        cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel
        cmake --install "$BUILD_DIR" --config "$CONFIG"
    done

    rm -rf "$BUILD_DIR"

    git -C "$SOURCE_DIR" rev-parse HEAD > "$HASH_FILE"
    echo "[$LIB_NAME] has been compiled."
}

copy_headers_only() {
    local LIB_NAME="$1"
    local SOURCE_DIR="$2"
    local INSTALL_DIR="$3"
    local HEADER_FILES=("${@:4}")
    local HASH_FILE="$INSTALL_DIR/git_hash.txt"

    if [ -d "$INSTALL_DIR" ] && check_git_hash_match "$SOURCE_DIR" "$HASH_FILE"; then
        echo "[$LIB_NAME] is up to date"
        return
    fi

    echo "Copying [$LIB_NAME] library files"

    rm -rf "$INSTALL_DIR"
    mkdir -p "$INSTALL_DIR"

    for HEADER in "${HEADER_FILES[@]}"; do
        cp "$SOURCE_DIR/$HEADER" "$INSTALL_DIR/"
    done

    git -C "$SOURCE_DIR" rev-parse HEAD > "$HASH_FILE"
    echo "[$LIB_NAME] has been copied"
}

build_library "SDL" \
    "$SRC_DIR/SDL" \
    "$BIN_DIR/SDL3"

build_library "Boost.Nowide" \
    "$SRC_DIR/Boost.Nowide" \
    "$BIN_DIR/nowide" \
    -DNOWIDE_INSTALL=ON

build_library "SDL_image" \
    "$SRC_DIR/SDL_image" \
    "$BIN_DIR/SDL3_image" \
    -DCMAKE_PREFIX_PATH=$BIN_DIR

build_library "glm" \
    "$SRC_DIR/glm" \
    "$BIN_DIR/glm" \
    -DGLM_BUILD_TESTS=OFF \
    -DGLM_ENABLE_CXX_20=ON

build_library "VulkanMemoryAllocator" \
    "$SRC_DIR/VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator" \
    "$BIN_DIR/VulkanMemoryAllocator" \
    -DVMA_BUILD_DOCUMENTATION=OFF \
    -DVMA_BUILD_SAMPLES=OFF

build_library "VulkanMemoryAllocator-Hpp" \
    "$SRC_DIR/VulkanMemoryAllocator-Hpp" \
    "$BIN_DIR/VulkanMemoryAllocator-Hpp" \
    -DVMA_HPP_ENABLE_INSTALL=ON

build_library "tinyobjloader" \
    "$SRC_DIR/tinyobjloader" \
    "$BIN_DIR/tinyobjloader"

echo "Done."
