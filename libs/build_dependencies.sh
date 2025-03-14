#!/usr/bin/bash

# Author: Grinlex

cd sources

if [ ! -d "../bin/posix/SDL3" ]; then
    echo "Compiling SDL"
    cd SDL
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DSDL_TESTS=OFF -DCMAKE_INSTALL_PREFIX="../../../bin/posix/SDL3" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/posix/glm" ]; then
    echo "Compiling glm"
    cd glm
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="../../../bin/posix/glm" $* ..
    cmake --build . -- all
    cmake --build . -- install
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/posix/VulkanMemoryAllocator" ]; then
    echo "Compiling VulkanMemoryAllocator"
    cd VulkanMemoryAllocator
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DVMA_BUILD_DOCUMENTATION=OFF -DVMA_BUILD_SAMPLES=OFF -DCMAKE_INSTALL_PREFIX="../../../bin/posix/VulkanMemoryAllocator" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/posix/VulkanMemoryAllocator-Hpp" ]; then
    echo "Compiling VulkanMemoryAllocator-Hpp"
    cd VulkanMemoryAllocator-Hpp
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DVMA_HPP_ENABLE_INSTALL=ON -DCMAKE_INSTALL_PREFIX="../../../bin/posix/VulkanMemoryAllocator-Hpp" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/posix/tinyobjloader" ]; then
    echo "Compiling tinyobjloader"
    cd tinyobjloader
    if [ -d "build" ]; then
        rm -rf build
    fi
    mkdir -p build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="../../../bin/posix/tinyobjloader" $* ..
    cmake --build . --config Release --parallel
    cmake --install . --config Release
    cd ../
    rm -rf build
    cd ../
fi

if [ ! -d "../bin/posix/stb" ]; then
    echo "Copying stb files"
    cd stb
    mkdir -p "../../bin/posix/stb"
    cp "./stb_image.h" "../../bin/posix/stb/"
    cd ../
fi


echo "Done."
