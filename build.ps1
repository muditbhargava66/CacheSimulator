# Build script for Cache Simulator on Windows
# PowerShell equivalent of build.sh

$ErrorActionPreference = "Stop"

# Get number of CPU cores
$NUM_CORES = [Environment]::ProcessorCount
if ($NUM_CORES -eq 0) { $NUM_CORES = 4 }

Write-Host "Building Cache Simulator v1.4.2" -ForegroundColor Cyan
Write-Host "Detected $NUM_CORES CPU cores" -ForegroundColor Gray

# Create build directory if it doesn't exist
if (-not (Test-Path "build")) {
  New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build

# Configure with CMake
Write-Host "`nConfiguring with CMake..." -ForegroundColor Yellow
cmake -DCMAKE_BUILD_TYPE=Release ..

if ($LASTEXITCODE -ne 0) {
  Write-Host "CMake configuration failed!" -ForegroundColor Red
  Set-Location ..
  exit 1
}

# Build
Write-Host "`nBuilding with $NUM_CORES parallel jobs..." -ForegroundColor Yellow
cmake --build . --parallel $NUM_CORES

if ($LASTEXITCODE -eq 0) {
  Write-Host "`nBuild successful!" -ForegroundColor Green
  Write-Host ""
  Write-Host "To run tests:" -ForegroundColor Cyan
  Write-Host "  ctest --output-on-failure"
  Write-Host ""
  Write-Host "To run the simulator:" -ForegroundColor Cyan
  Write-Host "  .\bin\cachesim.exe <trace_file>"
  Write-Host "  .\bin\cachesim.exe --config <config_file> <trace_file>"
  Write-Host ""
  Write-Host "Available tools:" -ForegroundColor Cyan
  Write-Host "  .\bin\tools\cache_analyzer.exe"
  Write-Host "  .\bin\tools\trace_generator.exe"
}
else {
  Write-Host "Build failed!" -ForegroundColor Red
  Set-Location ..
  exit 1
}

Set-Location ..
