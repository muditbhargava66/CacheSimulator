# Cache Simulator Tools

This directory contains utility tools for the Cache Simulator project.

## Available Tools

### Cache Analyzer
**File**: `cache_analyzer.cpp`

Analyzes cache behavior and provides recommendations for optimization.

```bash
# Basic usage
./bin/tools/cache_analyzer traces/trace1.txt

# With specific cache configuration
./bin/tools/cache_analyzer --config configs/high_performance.json traces/trace1.txt

# Generate optimization recommendations
./bin/tools/cache_analyzer --recommend traces/trace1.txt

# Export analysis to CSV
./bin/tools/cache_analyzer --export analysis.csv traces/trace1.txt
```

### Performance Comparison
**File**: `performance_comparison.cpp`

Compares performance of different cache configurations.

```bash
# Compare default configurations
./bin/tools/performance_comparison traces/trace1.txt

# Compare specific configurations
./bin/tools/performance_comparison --configs configs/high_performance.json,configs/write_optimized.json traces/trace1.txt

# Run parallel comparison
./bin/tools/performance_comparison --parallel 4 traces/trace1.txt

# Export results to CSV
./bin/tools/performance_comparison --export comparison.csv traces/trace1.txt
```

### Trace Generator
**File**: `trace_generator.cpp`

Generates synthetic memory access traces for testing and benchmarking.

```bash
# Generate sequential access pattern
./bin/tools/trace_generator --pattern sequential --num 10000 --output sequential_trace.txt

# Generate random access pattern
./bin/tools/trace_generator --pattern random --num 10000 --output random_trace.txt

# Generate strided pattern
./bin/tools/trace_generator --pattern strided --stride 128 --num 5000 --output strided_trace.txt

# Generate looping pattern
./bin/tools/trace_generator --pattern looping --loop-size 20 --repetitions 10 --output loop_trace.txt

# Generate mixed pattern with locality
./bin/tools/trace_generator --pattern mixed --regions 5 --locality 0.8 --num 10000 --output mixed_trace.txt

# Generate write-intensive pattern (80% writes)
./bin/tools/trace_generator --pattern sequential --write 0.8 --num 10000 --output write_heavy.txt

# Generate all standard trace patterns
./bin/tools/trace_generator --generate-all traces/
```

#### Trace Generator Options
- `--pattern`: sequential, strided, random, looping, mixed
- `--num`: Number of memory accesses
- `--output`: Output file name
- `--start`: Start address (hex format supported)
- `--end`: End address (hex format supported)
- `--stride`: Stride in bytes
- `--write`: Write ratio (0.0-1.0)
- `--loop-size`: Size of loop for looping pattern
- `--repetitions`: Number of repetitions
- `--regions`: Number of regions for mixed pattern
- `--region-size`: Size of each region in bytes
- `--locality`: Locality probability (0.0-1.0)
- `--seed`: Random seed for reproducibility

## Building the Tools

The tools are built automatically when building the main project:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

This will create the tool executables in the `bin/tools/` directory.

## Creating New Tools

To create a new tool:

1. Add a new `.cpp` file in the `tools/` directory
2. Update `CMakeLists.txt` to include the new tool
3. Build the project

Example CMake entry for a new tool:

```cmake
# Add new tool
add_executable(new_tool tools/new_tool.cpp)
target_link_libraries(new_tool cachesim_lib)
set_target_properties(new_tool PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tools)
```

## Tool Usage Examples

### Analyzing Cache Performance
```bash
# Analyze a trace with default configuration
./bin/tools/cache_analyzer traces/matrix_multiply.txt

# Compare different replacement policies
./bin/tools/performance_comparison --configs configs/nru_optimized.json,configs/high_performance.json traces/mixed_locality.txt
```

### Generating Test Traces
```bash
# Create a comprehensive test suite
./bin/tools/trace_generator --generate-all test_traces/

# Generate specific workload patterns
./bin/tools/trace_generator --pattern mixed --regions 10 --locality 0.9 --num 50000 --output high_locality.txt
./bin/tools/trace_generator --pattern random --num 20000 --write 0.1 --output read_heavy.txt
```

### Batch Analysis
```bash
# Analyze multiple traces with different configurations
for trace in traces/*.txt; do
    echo "Analyzing $trace"
    ./bin/tools/cache_analyzer --config configs/full_features.json "$trace" > "analysis_$(basename $trace .txt).txt"
done
```