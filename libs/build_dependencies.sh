#!/usr/bin/bash
set -e

cd sources/SDL
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DSDL_TESTS=OFF -DCMAKE_INSTALL_PREFIX="../../../SDL3" $* ..
cmake --build . --config Release --parallel
cmake --install . --config Release

cd ../../glm
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="../../../glm" $* ..
cmake --build . -- all
cmake --build . -- install

echo "Done."
