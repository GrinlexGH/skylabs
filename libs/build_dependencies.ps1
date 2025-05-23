# Author: Grinlex

param(
    [string]$Compiler = "msvc"
)

$ErrorActionPreference = "Stop"

$ROOT = $PSScriptRoot
$BIN_DIR = Join-Path $ROOT "bin/windows"
$SRC_DIR = Join-Path $ROOT "sources"

function Test-GitHashMatch {
    param(
        [string]$SourceDir,
        [string]$HashFile
    )

    $gitHash = (git -C $SourceDir rev-parse HEAD).Trim()

    if (Test-Path $HashFile) {
        $existingHash = (Get-Content $HashFile -First 1).Trim()
        return $gitHash -eq $existingHash
    }

    return $false
}

function New-Library {
    param(
        [string]$LibName,
        [string]$SourceDir,
        [string]$InstallBaseName,
        [string[]]$ExtraCMakeFlags = @()
    )

    $installDir = Join-Path $BIN_DIR $InstallBaseName
    $buildDir = Join-Path $SourceDir "build"
    $hashFile = Join-Path $installDir "git_hash.txt"

    if ((Test-Path $installDir) -and (Test-GitHashMatch $SourceDir $hashFile)) {
        Write-Host "[$LibName] is up to date."
        return
    }

    Write-Host "Compiling [$LibName]..."

    if (Test-Path $buildDir) { Remove-Item $buildDir -Recurse -Force }
    New-Item -Path $buildDir -ItemType Directory | Out-Null

    $cmakeArgs = @(
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX=$installDir",
        "-DCMAKE_PREFIX_PATH=$BIN_DIR;$BIN_DIR/static/$Compiler"
    ) + $ExtraCMakeFlags + @("-S", $SourceDir, "-B", $buildDir)

    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    & cmake --build $buildDir --config Release --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    if (Test-Path $InstallDir) { Remove-Item $InstallDir -Recurse -Force }
    New-Item -Path $InstallDir -ItemType Directory | Out-Null

    & cmake --install $buildDir --config Release
    if ($LASTEXITCODE -ne 0) { throw "Install failed" }

    Remove-Item $buildDir -Recurse -Force

    (git -C $SourceDir rev-parse HEAD).Trim() | Out-File $hashFile -Encoding ascii

    Write-Host "[$LibName] has been compiled."
}

function Get-CMakeGenerator {
    param([string]$Compiler)
    if ($Compiler -eq "msvc") {
        return $null
    } else {
        return "Ninja"
    }
}

function New-Static-Library {
    param(
        [string]$LibName,
        [string]$SourceDir,
        [string]$InstallBaseName,
        [string[]]$ExtraCMakeFlags = @()
    )

    $installDir = Join-Path $BIN_DIR "static/$Compiler/$InstallBaseName/"
    $buildDir = Join-Path $SourceDir "build"
    $hashFile = Join-Path $installDir "git_hash.txt"

    if ((Test-Path $installDir) -and (Test-GitHashMatch $SourceDir $hashFile)) {
        Write-Host "[$LibName] is up to date."
        return
    }

    Write-Host "Compiling [$LibName] static library with compiler: $Compiler..."

    $configs = @("Debug", "Release", "RelWithDebInfo", "MinSizeRel")
    $configPostfixMap = @{
        "Debug" = "_d"
        "Release" = ""
        "RelWithDebInfo" = "_rd"
        "MinSizeRel" = "_mr"
    }

    if (Test-Path $buildDir) { Remove-Item $buildDir -Recurse -Force }
    New-Item -Path $buildDir -ItemType Directory | Out-Null

    if (Test-Path $InstallDir) { Remove-Item $InstallDir -Recurse -Force }
    New-Item -Path $InstallDir -ItemType Directory | Out-Null

    $generator = Get-CMakeGenerator $Compiler

    foreach ($config in $configs) {
        $postfix = $configPostfixMap[$config]
        $configUpperName = $config.ToUpper()

        $cmakeArgs = @()
        if ($generator) {
            $cmakeArgs += "-G", $generator
        }

        $cmakeArgs += @(
            "-DCMAKE_BUILD_TYPE=$config",
            "-DCMAKE_INSTALL_PREFIX=$installDir",
            "-DCMAKE_PREFIX_PATH=$BIN_DIR;$BIN_DIR/static/$Compiler",
            "-DCMAKE_${configUpperName}_POSTFIX=`"$postfix`""
        ) + $ExtraCMakeFlags + @("-S", $SourceDir, "-B", $buildDir)

        & cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

        & cmake --build $buildDir --config $config --parallel
        if ($LASTEXITCODE -ne 0) { throw "Build failed" }

        & cmake --install $buildDir --config $config
        if ($LASTEXITCODE -ne 0) { throw "Install failed" }
    }

    Remove-Item $buildDir -Recurse -Force

    (git -C $SourceDir rev-parse HEAD).Trim() | Out-File $hashFile -Encoding ascii

    Write-Host "[$LibName] has been compiled."
}

# Build all libraries
New-Library "SDL" `
    (Join-Path $SRC_DIR "SDL") `
    ("SDL3")

New-Static-Library "Boost.Nowide" `
    (Join-Path $SRC_DIR "Boost.Nowide") `
    ("nowide") `
    @("-DNOWIDE_INSTALL=ON")

New-Library "SDL_Image" `
    (Join-Path $SRC_DIR "SDL_Image") `
    ("SDL3_Image") `

New-Static-Library "glm" `
    (Join-Path $SRC_DIR "glm") `
    ("glm") `
    @("-DGLM_BUILD_TESTS=OFF", "-DGLM_ENABLE_CXX_20=ON")

New-Library "VulkanMemoryAllocator" `
    (Join-Path $SRC_DIR "VulkanMemoryAllocator-Hpp/VulkanMemoryAllocator") `
    ("VulkanMemoryAllocator") `
    @("-DVMA_BUILD_DOCUMENTATION=OFF", "-DVMA_BUILD_SAMPLES=OFF")

New-Library "VulkanMemoryAllocator-Hpp" `
    (Join-Path $SRC_DIR "VulkanMemoryAllocator-Hpp") `
    ("VulkanMemoryAllocator-Hpp") `
    @("-DVMA_HPP_ENABLE_INSTALL=ON", "-DVMA_BUILD_EXAMPLE=OFF")

New-Static-Library "tinyobjloader" `
    (Join-Path $SRC_DIR "tinyobjloader") `
    ("tinyobjloader")

Write-Host "Done."
