@echo off
setlocal

rem Libraries
set sdl=SDL-preview-3.1.3
set glm=glm-1.0.1

rem Build SDL
cd sources\%sdl% || exit /b 1
mkdir build
cd build || exit /b 1
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="..\..\..\SDL3" .. || exit /b 1
cmake --build . --config Release --parallel || exit /b 1
cmake --install . --config Release || exit /b 1

rem Build GLM
cd ..\..\%glm% || exit /b 1
mkdir build
cd build || exit /b 1
cmake -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="..\..\..\glm" .. || exit /b 1
cmake --build . --config Release --parallel || exit /b 1
cmake --install . --config Release || exit /b 1

echo Done.
