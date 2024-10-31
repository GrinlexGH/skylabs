#!/usr/bin/bash

# Author: Grinlex

cd sources

if [ ! -d "../bin/linux/SDL3" ]; then
    echo "Compiling SDL"
    cd SDL
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DSDL_TESTS=OFF -DCMAKE_INSTALL_PREFIX="../../../bin/linux/SDL3" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/linux/glm" ]; then
    echo "Compiling glm"
    cd glm
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="../../../bin/linux/glm" $* ..
    cmake --build . -- all
    cmake --build . -- install
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/linux/VulkanMemoryAllocator" ]; then
    echo "Compiling VulkanMemoryAllocator"
    cd VulkanMemoryAllocator
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DVMA_BUILD_DOCUMENTATION=OFF -DVMA_BUILD_SAMPLES=OFF -DCMAKE_INSTALL_PREFIX="../../../bin/linux/VulkanMemoryAllocator" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/linux/VulkanMemoryAllocator-Hpp" ]; then
    echo "Compiling VulkanMemoryAllocator-Hpp"
    cd VulkanMemoryAllocator-Hpp
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DVMA_HPP_ENABLE_INSTALL=ON -DCMAKE_INSTALL_PREFIX="../../../bin/linux/VulkanMemoryAllocator-Hpp" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

echo "Done."
