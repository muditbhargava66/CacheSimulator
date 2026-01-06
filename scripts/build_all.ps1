# Comprehensive build script for Cache Simulator on Windows
# PowerShell equivalent of scripts/build_all.sh

param(
    [switch]$Debug,
    [switch]$Clean,
    [switch]$NoTests,
    [switch]$Docs,
    [int]$Jobs = [Environment]::ProcessorCount
)

$ErrorActionPreference = "Stop"

# Determine build type
$BUILD_TYPE = if ($Debug) { "Debug" } else { "Release" }

Write-Host "Cache Simulator Build Script (Windows)" -ForegroundColor Cyan
Write-Host "=======================================" -ForegroundColor Cyan
Write-Host "Build type: $BUILD_TYPE"
Write-Host "Clean build: $Clean"
Write-Host "Build tests: $(-not $NoTests)"
Write-Host "Build docs: $Docs"
Write-Host "Parallel jobs: $Jobs"
Write-Host ""

# Navigate to project root (assuming script is in scripts/ directory)
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
Set-Location $ProjectRoot

# Clean build directory if requested
if ($Clean -and (Test-Path "build")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build

# Configure with CMake
Write-Host "`nConfiguring with CMake..." -ForegroundColor Yellow
$CMAKE_ARGS = @("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")

if ($NoTests) {
    $CMAKE_ARGS += "-DBUILD_TESTING=OFF"
}

if ($Docs) {
    $CMAKE_ARGS += "-DBUILD_DOCS=ON"
}

& cmake $CMAKE_ARGS ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}

# Build
Write-Host "`nBuilding with $Jobs parallel jobs..." -ForegroundColor Yellow
cmake --build . --parallel $Jobs

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Set-Location $ProjectRoot
    exit 1
}

# Run tests if enabled
if (-not $NoTests) {
    Write-Host "`nRunning tests..." -ForegroundColor Yellow
    ctest --output-on-failure
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Some tests failed!" -ForegroundColor Yellow
    }
}

# Build documentation if enabled
if ($Docs) {
    Write-Host "`nBuilding documentation..." -ForegroundColor Yellow
    cmake --build . --target doc
}

Write-Host ""
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "To run the simulator:" -ForegroundColor Cyan
Write-Host "  .\bin\cachesim.exe <trace_file>"
Write-Host ""
Write-Host "To run with configuration:" -ForegroundColor Cyan
Write-Host "  .\bin\cachesim.exe --config <config_file> <trace_file>"
Write-Host ""
Write-Host "Available tools:" -ForegroundColor Cyan
Write-Host "  .\bin\tools\cache_analyzer.exe"
Write-Host "  .\bin\tools\performance_comparison.exe"
Write-Host "  .\bin\tools\trace_generator.exe"

Set-Location $ProjectRoot
