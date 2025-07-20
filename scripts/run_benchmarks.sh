#!/bin/bash
# Comprehensive benchmark script for Cache Simulator
# Runs a series of benchmarks with different configurations and traces

# Ensure we're in the project root directory
if [ ! -d "build" ] || [ ! -d "traces" ] || [ ! -d "configs" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Check if simulator is built
if [ ! -f "build/bin/cachesim" ]; then
    echo "Error: Simulator not built. Please build the project first."
    echo "Run: mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

# Create results directory
RESULTS_DIR="benchmark_results"
mkdir -p "$RESULTS_DIR"

echo "Cache Simulator Benchmark Suite"
echo "==============================="
echo "Starting benchmarks at $(date)"
echo ""

# Function to run benchmark and save results
run_benchmark() {
    local name=$1
    local config=$2
    local trace=$3
    local options=$4
    
    echo "Running benchmark: $name"
    echo "  Config: $config"
    echo "  Trace: $trace"
    echo "  Options: $options"
    
    # Create output file name
    local output_file="${RESULTS_DIR}/${name// /_}.txt"
    
    # Run benchmark
    if [ -n "$config" ]; then
        build/bin/cachesim --config "$config" $options "$trace" > "$output_file"
    else
        build/bin/cachesim $options "$trace" > "$output_file"
    fi
    
    # Extract key metrics
    local hit_rate=$(grep "L1 Hit Ratio" "$output_file" | awk '{print $NF}')
    local processing_time=$(grep "Processing Time" "$output_file" | awk '{print $3}')
    
    echo "  Results:"
    echo "    Hit Rate: $hit_rate"
    echo "    Processing Time: $processing_time ms"
    echo ""
}

echo "Basic Benchmarks"
echo "---------------"

# Run basic benchmarks
run_benchmark "Default Config" "" "traces/trace1.txt" ""
run_benchmark "High Performance" "configs/high_performance.json" "traces/trace1.txt" ""
run_benchmark "Write Optimized" "configs/write_optimized.json" "traces/write_intensive.txt" ""

echo "Feature Benchmarks"
echo "-----------------"

# Run feature benchmarks
run_benchmark "Victim Cache" "configs/victim_cache_config.json" "traces/conflict_heavy.txt" ""
run_benchmark "NRU Policy" "configs/nru_optimized.json" "traces/mixed_locality.txt" ""
run_benchmark "Parallel Processing" "configs/high_performance.json" "traces/comprehensive_test.txt" "--parallel"

echo "Advanced Benchmarks"
echo "------------------"

# Run advanced benchmarks
run_benchmark "Multi-processor" "configs/multiprocessor_4core.json" "traces/multiprocessor_coherence.txt" ""
run_benchmark "Full Features" "configs/full_features.json" "traces/advanced_features.txt" "--victim-cache --parallel"

echo "Benchmark Summary"
echo "================="
echo "All benchmarks completed at $(date)"
echo "Results saved to $RESULTS_DIR/"
echo ""
echo "To compare results, use:"
echo "  ./build/bin/tools/performance_comparison --configs configs/high_performance.json,configs/write_optimized.json traces/trace1.txt"