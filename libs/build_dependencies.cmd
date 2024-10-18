@echo off
setlocal

rem Build SDL
cd sources\SDL
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="..\..\..\SDL3" %* .. || exit /b 1
cmake --build . --config Release --parallel || exit /b 1
cmake --install . --config Release || exit /b 1

rem Build GLM
cd ..\..\glm
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="..\..\..\glm" %* .. || exit /b 1
cmake --build . --config Release --parallel || exit /b 1
cmake --install . --config Release || exit /b 1

echo Done.
