#!/bin/bash
# Comprehensive build script for Cache Simulator
# Builds the project with different configurations

# Default values
BUILD_TYPE="Release"
CLEAN=false
TESTS=true
DOCS=false
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --no-tests)
            TESTS=false
            shift
            ;;
        --docs)
            DOCS=true
            shift
            ;;
        --jobs=*)
            JOBS="${key#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $key"
            echo "Usage: $0 [--debug] [--clean] [--no-tests] [--docs] [--jobs=N]"
            exit 1
            ;;
    esac
done

echo "Cache Simulator Build Script"
echo "==========================="
echo "Build type: $BUILD_TYPE"
echo "Clean build: $CLEAN"
echo "Build tests: $TESTS"
echo "Build docs: $DOCS"
echo "Parallel jobs: $JOBS"
echo ""

# Create build directory
if [ "$CLEAN" = true ] && [ -d "build" ]; then
    echo "Cleaning build directory..."
    rm -rf build
fi

mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

if [ "$TESTS" = false ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_TESTING=OFF"
fi

if [ "$DOCS" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DBUILD_DOCS=ON"
fi

cmake $CMAKE_ARGS ..

# Build
echo "Building with $JOBS parallel jobs..."
cmake --build . -j$JOBS

# Run tests if enabled
if [ "$TESTS" = true ]; then
    echo "Running tests..."
    ctest --output-on-failure
fi

# Build documentation if enabled
if [ "$DOCS" = true ]; then
    echo "Building documentation..."
    cmake --build . --target doc
fi

echo ""
echo "Build completed successfully!"
echo ""
echo "To run the simulator:"
echo "  ./bin/cachesim <trace_file>"
echo ""
echo "To run with configuration:"
echo "  ./bin/cachesim --config <config_file> <trace_file>"
echo ""
echo "Available tools:"
echo "  ./bin/tools/cache_analyzer"
echo "  ./bin/tools/performance_comparison"
echo "  ./bin/tools/trace_generator"