#!/usr/bin/bash

# Author: Grinlex

cd sources

if [ ! -d "../bin/unix/SDL3" ]; then
    echo "Compiling SDL"
    cd SDL
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSDL_TESTS=OFF -DCMAKE_INSTALL_PREFIX="../../../bin/unix/SDL3" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/unix/glm" ]; then
    echo "Compiling glm"
    cd glm
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="../../../bin/unix/glm" $* ..
    cmake --build . -- all
    cmake --build . -- install
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/unix/VulkanMemoryAllocator" ]; then
    echo "Compiling VulkanMemoryAllocator"
    cd VulkanMemoryAllocator
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DVMA_BUILD_DOCUMENTATION=OFF -DVMA_BUILD_SAMPLES=OFF -DCMAKE_INSTALL_PREFIX="../../../bin/unix/VulkanMemoryAllocator" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/unix/VulkanMemoryAllocator-Hpp" ]; then
    echo "Compiling VulkanMemoryAllocator-Hpp"
    cd VulkanMemoryAllocator-Hpp
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DVMA_HPP_ENABLE_INSTALL=ON -DCMAKE_INSTALL_PREFIX="../../../bin/unix/VulkanMemoryAllocator-Hpp" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/unix/tinyobjloader" ]; then
    echo "Compiling tinyobjloader"
    cd tinyobjloader
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="../../../bin/unix/tinyobjloader" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/unix/stb" ]; then
    echo "Copying stb files"
    cd stb
    mkdir -p "../../bin/unix/stb"
    cp "./stb_image.h" "../../bin/unix/stb/"
    cd ../
fi


echo "Done."
