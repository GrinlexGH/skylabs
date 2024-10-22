@echo off
setlocal

rem Author: Grinlex

cd sources

if not exist "..\bin\windows\SDL3" (
    echo Compiling SDL
    cd SDL
    if exist build (
        rmdir /s /q build
    )
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="..\..\..\bin\windows\SDL3" %* .. || exit /b 1
    cmake --build . --config Release --parallel || exit /b 1
    cmake --install . --config Release || exit /b 1
    cd ..\..\
)

if not exist "..\bin\windows\glm" (
    echo Compiling glm
    cd glm
    if exist build (
        rmdir /s /q build
    )
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="..\..\..\bin\windows\glm" %* .. || exit /b 1
    cmake --build . --config Release --parallel || exit /b 1
    cmake --install . --config Release || exit /b 1
    cd ..\..\
)

echo Done.
