#!/usr/bin/bash
set -e

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
    cd ../../
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
    cd ../../
fi

echo "Done."
