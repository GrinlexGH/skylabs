#!/usr/bin/bash

# libraries
sdl=SDL-preview-3.1.3
glm=glm-1.0.1

cd sources/$sdl
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../../SDL3 ..
cmake --build . --config Release --parallel
cmake --install . --config Release

cd ../../$glm
mkdir -p build
cd build
cmake -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX=../../../glm ..
cmake --build . -- all
cmake --build . -- install

