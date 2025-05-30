#!/bin/bash
set -euo pipefail

# Author: Grinlex

ROOT="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR="$ROOT/bin/unix"
SRC_DIR="$ROOT/sources"

check_git_hash_match() {
    local source_dir="$1"
    local hash_file="$2"

    local git_hash
    git_hash=$(git -C "$source_dir" rev-parse HEAD)

    if [ -f "$hash_file" ]; then
        local existing_hash
        read -r existing_hash < "$hash_file"
        [ "$git_hash" == "$existing_hash" ]
        return
    fi
    return 1
}

build_library() {
    local lib_name="$1"
    local source_dir="$2"
    local install_base_name="$3"
    local extra_cmake_flags=("${@:4}")

    local install_dir="$BIN_DIR/$install_base_name"
    local build_dir="$source_dir/build"
    local hash_file="$install_dir/git_hash.txt"

    if [ -d "$install_dir" ] && check_git_hash_match "$source_dir" "$hash_file"; then
        echo "[$lib_name] is up to date."
        return
    fi

    echo "Compiling [$lib_name]..."

    rm -rf "$build_dir"
    mkdir -p "$build_dir"

    cmake_args=(
        -G Ninja
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX="$install_dir"
        -DCMAKE_PREFIX_PATH="$BIN_DIR;$BIN_DIR/static"
        "${extra_cmake_flags[@]}"
        -S "$source_dir"
        -B "$build_dir"
    )

    cmake "${cmake_args[@]}"
    cmake --build "$build_dir" --config Release --parallel

    rm -rf "$install_dir"
    mkdir -p "$install_dir"

    cmake --install "$build_dir" --config Release
    rm -rf "$build_dir"

    git -C "$source_dir" rev-parse HEAD > "$hash_file"

    echo "[$lib_name] has been compiled."
}

build_static_library() {
    local lib_name="$1"
    local source_dir="$2"
    local install_base_name="$3"
    local extra_cmake_flags=("${@:4}")

    local install_dir="$BIN_DIR/static/$install_base_name"
    local build_dir="$source_dir/build"
    local hash_file="$install_dir/git_hash.txt"

    if [ -d "$install_dir" ] && check_git_hash_match "$source_dir" "$hash_file"; then
        echo "[$lib_name] is up to date."
        return
    fi

    echo "Compiling [$lib_name] static library..."

    declare -A config_postfix_map=(
        ["Debug"]="_d"
        ["Release"]=""
        ["RelWithDebInfo"]="_rd"
        ["MinSizeRel"]="_mr"
    )

    local configs=("Debug" "Release" "RelWithDebInfo" "MinSizeRel")

    rm -rf "$build_dir" "$install_dir"
    mkdir -p "$build_dir" "$install_dir"

    for config in "${configs[@]}"; do
        local postfix="${config_postfix_map[$config]}"
        local config_upper
        config_upper=$(echo "$config" | tr '[:lower:]' '[:upper:]')

        cmake_args=(
            -G Ninja
            -DCMAKE_BUILD_TYPE="$config"
            -DCMAKE_INSTALL_PREFIX="$install_dir"
            -DCMAKE_PREFIX_PATH="$BIN_DIR;$BIN_DIR/static"
            "-DCMAKE_${config_upper}_POSTFIX=${postfix}"
            "${extra_cmake_flags[@]}"
            -S "$source_dir"
            -B "$build_dir"
        )

        cmake "${cmake_args[@]}"
        cmake --build "$build_dir" --config "$config" --parallel
        cmake --install "$build_dir" --config "$config"
    done

    rm -rf "$build_dir"
    git -C "$source_dir" rev-parse HEAD > "$hash_file"

    echo "[$lib_name] has been compiled."
}

# Build all libraries
build_library "SDL" \
    "$SRC_DIR/SDL" \
    "SDL3"

build_library "SDL_image" \
    "$SRC_DIR/SDL_image" \
    "SDL3_image"

build_static_library "Boost.Nowide" \
    "$SRC_DIR/Boost.Nowide" \
    "nowide" \
    -DNOWIDE_INSTALL=ON

build_static_library "glm" \
    "$SRC_DIR/glm" \
    "glm" \
    -DGLM_BUILD_TESTS=OFF \
    -DGLM_ENABLE_CXX_20=ON

build_library "VulkanMemoryAllocator" \
    "$SRC_DIR/VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator" \
    "VulkanMemoryAllocator" \
    -DVMA_BUILD_DOCUMENTATION=OFF \
    -DVMA_BUILD_SAMPLES=OFF

build_library "VulkanMemoryAllocator-Hpp" \
    "$SRC_DIR/VulkanMemoryAllocator-Hpp" \
    "VulkanMemoryAllocator-Hpp" \
    -DVMA_HPP_ENABLE_INSTALL=ON \
    -DVMA_BUILD_EXAMPLE=OFF

build_static_library "tinyobjloader" \
    "$SRC_DIR/tinyobjloader" \
    "tinyobjloader"

echo "Done."
