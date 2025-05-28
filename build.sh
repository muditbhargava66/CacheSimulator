#!/bin/bash
# Build script for Cache Simulator
# Works on both Linux and macOS

# Detect OS and get number of CPU cores
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    NUM_CORES=$(sysctl -n hw.ncpu)
else
    # Linux and others
    NUM_CORES=$(nproc 2>/dev/null || echo 4)
fi

echo "Building Cache Simulator v1.1.0"
echo "Detected $NUM_CORES CPU cores"

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
echo "Building with $NUM_CORES parallel jobs..."
cmake --build . -j$NUM_CORES

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo ""
    echo "To run tests:"
    echo "  ctest --verbose"
    echo ""
    echo "To run the simulator:"
    echo "  ./bin/cachesim <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <PREF_N> <PREF_M> <trace_file>"
else
    echo "Build failed!"
    exit 1
fi
