# Libraries
$sdl = "SDL-preview-3.1.3"
$glm = "glm-1.0.1"

function Test-LastExitCode {
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Command failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }
}

# Build SDL
Set-Location "sources\$sdl" -ErrorAction Stop
New-Item -ItemType Directory -Force -Name build
Set-Location build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="..\..\..\SDL3" ..
Test-LastExitCode
cmake --build . --config Release --parallel
Test-LastExitCode
cmake --install . --config Release
Test-LastExitCode

# Build GLM
Set-Location "../../$glm" -ErrorAction Stop
New-Item -ItemType Directory -Force -Name build
Set-Location build
cmake -DCMAKE_BUILD_TYPE=Release -DGLM_BUILD_TESTS=OFF -DGLM_ENABLE_CXX_20=ON -DCMAKE_INSTALL_PREFIX="..\..\..\glm" ..
Test-LastExitCode
cmake --build . --config Release --parallel
Test-LastExitCode
cmake --install . --config Release
Test-LastExitCode

Set-Location "..\..\..\"

Write-Output "Done."
