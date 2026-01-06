# Building the Cache Simulator

## Requirements

### Compilers

| Platform | Compiler | Minimum Version |
|----------|----------|-----------------|
| Linux | GCC | 10.0+ |
| Linux | Clang | 10.0+ |
| macOS | Clang | 12.0+ |
| Windows | MSVC | 2019 (16.8+) |
| Windows | MinGW-w64 | GCC 10+ |

### Build Tools

- CMake 3.14 or higher
- Make, Ninja, or MSBuild

### Dependencies

All dependencies are header-only and included in the repository:

- C++20 Standard Library
- No external runtime dependencies

## Build Instructions

### Linux/macOS

```bash
# Clone repository
git clone https://github.com/muditbhargava66/CacheSimulator.git
cd CacheSimulator

# Create build directory
mkdir build && cd build

# Configure (Release build)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo cmake --install .
```

### Windows (MSVC)

```powershell
# Using PowerShell script
.\build.ps1

# Or manually with CMake
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

### Windows (MinGW)

```bash
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j8
```

## Build Options

| CMake Option | Default | Description |
|--------------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug, Release, RelWithDebInfo) |
| `BUILD_TESTS` | ON | Build unit and integration tests |
| `BUILD_DOCS` | OFF | Build Doxygen documentation |

### Example with Options

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
```

## Build Outputs

After successful build:

```
build/
  bin/
    cachesim              # Main executable
    tests/
      unit/               # Unit test executables
      integration/        # Integration tests
      performance/        # Performance benchmarks
```

## Running Tests

```bash
# All tests
cd build
ctest --output-on-failure

# Specific test category
ctest -R unit              # Unit tests only
ctest -R integration       # Integration tests only
ctest -R performance       # Performance tests only

# Verbose output
ctest -V
```

## Troubleshooting

### CMake Cannot Find Compiler

```bash
# Specify compiler explicitly
cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++-11 ..
```

### Missing C++20 Support

Ensure your compiler supports C++20:

```bash
g++ --version   # Should be 10.0+
clang++ --version  # Should be 10.0+
```

### Windows Build Errors

See [WINDOWS.md](../WINDOWS.md) for platform-specific troubleshooting.

## Continuous Integration

The project uses GitHub Actions for CI:

- Build verification on Linux, macOS, Windows
- Unit and integration testing
- Code coverage reporting

## Development Build

For development with debug symbols:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j$(nproc)
```

Enable address sanitizer (GCC/Clang):

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
```
