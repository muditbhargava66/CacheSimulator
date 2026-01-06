# Run benchmarks script for Cache Simulator on Windows
# PowerShell equivalent of scripts/run_benchmarks.sh

param(
    [string]$TraceDir = "traces",
    [string]$OutputDir = "results",
    [switch]$Verbose
)

$ErrorActionPreference = "Stop"

Write-Host "Cache Simulator Benchmark Runner (Windows)" -ForegroundColor Cyan
Write-Host "===========================================" -ForegroundColor Cyan

# Navigate to project root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
Set-Location $ProjectRoot

# Check if simulator exists
$SimulatorPath = "build\bin\cachesim.exe"
if (-not (Test-Path $SimulatorPath)) {
    Write-Host "Error: Simulator not found at $SimulatorPath" -ForegroundColor Red
    Write-Host "Please build the project first: .\build.ps1" -ForegroundColor Yellow
    exit 1
}

# Create output directory
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

# Find trace files
$TraceFiles = Get-ChildItem -Path $TraceDir -Filter "*.txt" -ErrorAction SilentlyContinue

if ($TraceFiles.Count -eq 0) {
    Write-Host "No trace files found in $TraceDir" -ForegroundColor Yellow
    exit 1
}

Write-Host "Found $($TraceFiles.Count) trace files"
Write-Host ""

# Configurations to benchmark
$Configs = @(
    @{Name = "Small L1"; L1Size = 16384; L1Assoc = 2; L2Size = 0; L2Assoc = 0 },
    @{Name = "Default"; L1Size = 32768; L1Assoc = 4; L2Size = 262144; L2Assoc = 8 },
    @{Name = "Large L2"; L1Size = 32768; L1Assoc = 4; L2Size = 524288; L2Assoc = 16 }
)

# Timestamp for results
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$ResultFile = Join-Path $OutputDir "benchmark_$Timestamp.csv"

# CSV Header
"Trace,Config,L1_Size,L1_Assoc,L2_Size,L2_Assoc,Time_ms" | Out-File $ResultFile

foreach ($TraceFile in $TraceFiles) {
    Write-Host "Processing: $($TraceFile.Name)" -ForegroundColor Yellow
    
    foreach ($Config in $Configs) {
        $StartTime = Get-Date
        
        # Pre-calculate block size (avoiding arithmetic in array literal)
        $BlockSize = [int]($Config.L1Size / 64)
        
        $SimArgs = @(
            $BlockSize,
            $Config.L1Size,
            $Config.L1Assoc,
            $Config.L2Size,
            $Config.L2Assoc,
            0,  # Prefetch disabled
            0,
            $TraceFile.FullName
        )
        
        if ($Verbose) {
            & $SimulatorPath $SimArgs
        }
        else {
            & $SimulatorPath $SimArgs | Out-Null
        }
        
        $Duration = ((Get-Date) - $StartTime).TotalMilliseconds
        
        "$($TraceFile.Name),$($Config.Name),$($Config.L1Size),$($Config.L1Assoc),$($Config.L2Size),$($Config.L2Assoc),$([math]::Round($Duration, 2))" | Out-File $ResultFile -Append
        
        Write-Host "  $($Config.Name): $([math]::Round($Duration, 2)) ms" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "Benchmark complete! Results saved to: $ResultFile" -ForegroundColor Green
