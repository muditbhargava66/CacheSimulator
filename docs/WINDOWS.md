# Windows Platform Support

> **Version**: 1.4.1
> **Last Updated**: January 2026

This document covers building and running CacheSimulator on Windows systems.

## Prerequisites

### Required
- **CMake 3.14+** - Download from [cmake.org](https://cmake.org/download/)
- **C++20 compiler** - One of:
  - **GCC 10+** (via MinGW-w64 or MSYS2)
  - **MSVC 2019+** (Visual Studio)
  - **Clang 10+**

### Recommended
- **Ninja build system** - Faster parallel builds
- **Git** - For version control

## Build Instructions

### Using GCC (MinGW/MSYS2)

```powershell
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . -j8
```

### Using Visual Studio

```powershell
# Configure with MSVC generator
cmake -G "Visual Studio 17 2022" -A x64 ..

# Build
cmake --build . --config Release
```

### Using Ninja (Recommended)

```powershell
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Running the Simulator

```powershell
# Run with a trace file
.\bin\cachesim.exe ..\traces\trace1.txt

# Run with custom configuration
.\bin\cachesim.exe --config ..\configs\high_performance.json ..\traces\matrix_multiply.txt

# Run cache analyzer
.\bin\tools\cache_analyzer.exe ..\traces\trace1.txt

# Run trace generator
.\bin\tools\trace_generator.exe -p sequential -n 1000 -o my_trace.txt
```

## Running Tests

```powershell
# Run all tests
ctest --output-on-failure

# Run specific test category
ctest -R unit
ctest -R performance
```

## Known Issues and Solutions

### 1. C++20 Requirement

**Issue**: Build fails with errors about designated initializers.

**Solution**: The CMakeLists.txt now requires C++20. Ensure your compiler supports it:
- GCC 10+
- MSVC 2019 16.11+
- Clang 10+

### 2. File Locking in Tests

**Issue**: `visualization_test` may fail with "file is being used by another process".

**Solution**: Fixed in v1.2.2. File streams are now properly closed using RAII scope blocks before calling `std::filesystem::remove()`.

### 3. Console Encoding (Unicode)

**Issue**: Unicode characters (✓, ✗) may not display correctly in Windows Command Prompt.

**Solution**: Use Windows Terminal or PowerShell which support UTF-8:
```powershell
# Set UTF-8 encoding in PowerShell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
```

### 4. ANSI Color Codes

**Issue**: Colors may not appear in legacy Command Prompt.

**Solution**: Use Windows Terminal, PowerShell, or a terminal that supports ANSI escape codes. The simulator auto-detects color support and falls back to plain text.

## PowerShell Build Scripts

Windows-native PowerShell scripts are provided for convenience:

### Quick Build
```powershell
# From project root
.\build.ps1
```

### Comprehensive Build
```powershell
# From project root
.\scripts\build_all.ps1 [options]

# Options:
#   -Debug      Build with debug symbols
#   -Clean      Clean build directory first
#   -NoTests    Skip building tests
#   -Docs       Build documentation
#   -Jobs N     Number of parallel jobs (default: CPU cores)

# Examples:
.\scripts\build_all.ps1 -Debug -Clean
.\scripts\build_all.ps1 -NoTests -Jobs 8
```

### Benchmark Runner
```powershell
.\scripts\run_benchmarks.ps1 [-Verbose]

# Results saved to: results\benchmark_YYYYMMDD_HHMMSS.csv
```

## Performance Notes

- **Parallel builds**: Use `--parallel 8` or higher for faster compilation
- **Release builds**: Use `CMAKE_BUILD_TYPE=Release` for optimal performance
- **Ninja generator**: Faster than MSVC generator for incremental builds

## Compatibility Matrix

| Feature             | Windows   | macOS | Linux |
| ------------------- | --------- | ----- | ----- |
| Core simulation     | ✓         | ✓     | ✓     |
| Multi-processor     | ✓         | ✓     | ✓     |
| Parallel processing | ✓         | ✓     | ✓     |
| File operations     | ✓         | ✓     | ✓     |
| Console colors      | Terminal¹ | ✓     | ✓     |
| All tests pass      | ✓         | ✓     | ✓     |

¹ Requires Windows Terminal or compatible terminal emulator

## Troubleshooting

### Build Configuration Errors

If CMake fails to configure, verify:
```powershell
cmake --version  # Should be 3.14+
g++ --version    # Should be GCC 10+ for C++20
```

### Missing Headers

If you see errors like "'array' file not found", this is an IDE issue, not a compiler issue. The actual GCC/MSVC build should work correctly.

### Test Failures

If tests fail, run with verbose output:
```powershell
ctest --output-on-failure -V
```

## Contact

For Windows-specific issues, please open a GitHub issue with:
- Windows version
- Compiler version
- CMake version
- Full error output
