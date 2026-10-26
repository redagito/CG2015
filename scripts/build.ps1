[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$GpuTests,
    [switch]$Package
)
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$oldConan = $env:CONAN_HOME
$oldTemp = $env:TEMP
$oldTmp = $env:TMP
Push-Location $project
try {
    $env:CONAN_HOME = Join-Path $project '.conan2'
    $env:TEMP = Join-Path $project 'build/tmp'
    $env:TMP = $env:TEMP
    New-Item -ItemType Directory -Force $env:TEMP | Out-Null
    New-Item -ItemType Directory -Force (Join-Path $env:CONAN_HOME 'profiles') | Out-Null
    Copy-Item -LiteralPath (Join-Path $project 'profiles/windows-msvc') -Destination (Join-Path $env:CONAN_HOME 'profiles/default')
    conan --version
    cmake --version
    conan install . -pr:h=profiles/windows-msvc -pr:b=profiles/windows-msvc -s:h "build_type=$Configuration" --build=missing
    if ($LASTEXITCODE) { throw 'Conan install failed.' }
    $gpu = if ($GpuTests) { 'ON' } else { 'OFF' }
    cmake --preset conan-default "-DCG2015_GPU_TESTS=$gpu"
    if ($LASTEXITCODE) { throw 'CMake configure failed.' }
    cmake --build build --config $Configuration --parallel
    if ($LASTEXITCODE) { throw 'Build failed.' }
    ctest --test-dir build -C $Configuration --output-on-failure
    if ($LASTEXITCODE) { throw 'Tests failed.' }
    if ($Package) {
        cmake --install build --config $Configuration --prefix "build/package/$Configuration"
        if ($LASTEXITCODE) { throw 'Packaging failed.' }
    }
} finally {
    Pop-Location
    $env:CONAN_HOME = $oldConan
    $env:TEMP = $oldTemp
    $env:TMP = $oldTmp
}
