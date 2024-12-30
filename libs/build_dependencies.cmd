@echo off
setlocal

rem Author: Grinlex

cd sources

if not exist "..\bin\windows\SDL3" (
    echo Compiling SDL
    mkdir "..\bin\windows\SDL3"
    cd SDL
    if exist build (
        rmdir /s /q build
    )
    mkdir build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="..\..\..\bin\windows\SDL3" %* .. || exit /b 1
    cmake --build . --config Release --parallel || exit /b 1
    cmake --install . --config Release || exit /b 1
    cd ..\
    rmdir /s /q build
    cd ..
)

if not exist "..\bin\windows\glm" (
    echo Compiling glm
    mkdir "..\bin\windows\glm"
    cd glm
    if exist build (
        rmdir /s /q build
    )
    mkdir build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="..\..\..\bin\windows\glm" %* .. || exit /b 1
    cmake --build . --config Release --parallel || exit /b 1
    cmake --install . --config Release || exit /b 1
    cd ..\
    rmdir /s /q build
    cd ..
)

if not exist "..\bin\windows\VulkanMemoryAllocator" (
    echo Compiling VulkanMemoryAllocator
    mkdir "..\bin\windows\VulkanMemoryAllocator"
    cd VulkanMemoryAllocator
    if exist build (
        rmdir /s /q build
    )
    mkdir build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DVMA_BUILD_DOCUMENTATION=OFF -DVMA_BUILD_SAMPLES=OFF -DCMAKE_INSTALL_PREFIX="..\..\..\bin\windows\VulkanMemoryAllocator" %* .. || exit /b 1
    cmake --install . --config Release || exit /b 1
    cd ..\
    rmdir /s /q build
    cd ..
)

if not exist "..\bin\windows\VulkanMemoryAllocator-Hpp" (
    echo Compiling VulkanMemoryAllocator-Hpp
    mkdir "..\bin\windows\VulkanMemoryAllocator-Hpp"
    cd VulkanMemoryAllocator-Hpp
    if exist build (
        rmdir /s /q build
    )
    mkdir build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_BUILD_TYPE=Release -DVMA_HPP_ENABLE_INSTALL=ON -DCMAKE_INSTALL_PREFIX="..\..\..\bin\windows\VulkanMemoryAllocator-Hpp" %* .. || exit /b 1
    cmake --install . --config Release || exit /b 1
    cd ..\
    rmdir /s /q build
    cd ..
)

if not exist "..\bin\windows\tinyobjloader" (
    echo Compiling tinyobjloader
    mkdir "..\bin\windows\tinyobjloader"
    cd tinyobjloader
    if exist build (
        rmdir /s /q build
    )
    mkdir build
    cd build
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="..\..\..\bin\windows\tinyobjloader" %* .. || exit /b 1
    cmake --build . --config Release --parallel || exit /b 1
    cmake --install . --config Release || exit /b 1
    cd ..\
    rmdir /s /q build
    cd ..
)

if not exist "..\bin\windows\stb" (
    echo Copying stb files
    cd stb
    robocopy . "..\..\bin\windows\stb" stb_image.h /mt /z || exit /b 1
    cd ..
)

echo Done.
