#!/bin/bash
# Quick build script for macOS
# Detects the number of cores and builds the project

# Get number of CPU cores on macOS
NUM_CORES=$(sysctl -n hw.ncpu)

echo "Building Cache Simulator on macOS with $NUM_CORES cores..."
echo ""

# Ensure we're in the build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure if needed
if [ ! -f "Makefile" ]; then
    echo "Configuring with CMake..."
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

# Build
echo "Building..."
cmake --build . -j$NUM_CORES

# Check result
if [ $? -eq 0 ]; then
    echo ""
    echo "Build successful!"
    echo ""
    echo "To run the simulator:"
    echo "  ./bin/cachesim <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <PREF_N> <PREF_M> <trace_file>"
    echo ""
    echo "Example:"
    echo "  ./bin/cachesim 64 32768 4 262144 8 1 4 ../traces/trace1.txt"
else
    echo ""
    echo "Build failed! Please check the error messages above."
    exit 1
fi
