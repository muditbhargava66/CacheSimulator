# Cache Simulator Tutorial

## Table of Contents
1. [Getting Started](#getting-started)
2. [Basic Usage](#basic-usage)
3. [Advanced Features](#advanced-features)
4. [Performance Tuning](#performance-tuning)
5. [Multi-Processor Simulation](#multi-processor-simulation)
6. [Visualization and Analysis](#visualization-and-analysis)
7. [Best Practices](#best-practices)

## Getting Started

### Installation

```bash
# Clone the repository
git clone https://github.com/muditbhargava66/CacheSimulator.git
cd CacheSimulator

# Build with CMake (recommended)
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)

# Or build with Make
make -j$(nproc)
```

### Quick Start

```bash
# Run with default configuration
./build/bin/cachesim traces/simple.txt

# Run with custom parameters
./build/bin/cachesim 64 32768 4 262144 8 1 4 traces/complex.txt
#                    BS  L1   A1  L2    A2 P  D
# BS = Block Size, L1 = L1 Size, A1 = L1 Associativity
# L2 = L2 Size, A2 = L2 Associativity, P = Prefetch, D = Distance
```

## Basic Usage

### Understanding Cache Parameters

#### Cache Size
The total capacity of the cache in bytes. Common sizes:
- L1: 32KB - 64KB
- L2: 256KB - 1MB
- L3: 8MB - 32MB

```cpp
// Example: 32KB L1 cache
CacheConfig l1Config;
l1Config.size = 32768;  // 32 * 1024 bytes
```

#### Associativity
Number of ways in each set. Higher associativity reduces conflicts:
- Direct-mapped: 1-way (simple, fast)
- Set-associative: 2, 4, 8, 16-way (balanced)
- Fully-associative: N-way (complex, flexible)

```cpp
// 4-way set associative
l1Config.associativity = 4;
```

#### Block Size
Size of each cache line in bytes. Typically 64 or 128 bytes:

```cpp
l1Config.blockSize = 64;  // Standard x86 cache line
```

### Creating Trace Files

Trace files contain memory access patterns:

```
r 0x1000    # Read from address 0x1000
w 0x2000    # Write to address 0x2000
r 0x1040    # Read from address 0x1040
```

Generate traces programmatically:

```cpp
// trace_generator.cpp
std::ofstream trace("workload.txt");

// Sequential access pattern
for (uint32_t addr = 0x1000; addr < 0x2000; addr += 64) {
    trace << "r 0x" << std::hex << addr << std::endl;
}

// Random access pattern
std::mt19937 rng(42);
std::uniform_int_distribution<uint32_t> dist(0x1000, 0x10000);
for (int i = 0; i < 1000; ++i) {
    trace << (i % 4 == 0 ? "w " : "r ") 
          << "0x" << std::hex << (dist(rng) & ~63) << std::endl;
}
```

### Running Simulations

#### Command Line Mode

```bash
# Basic simulation
./cachesim -c config.json traces/workload.txt

# With visualization
./cachesim --visualize traces/workload.txt

# Parallel simulation (v1.2.0)
./cachesim -p 4 traces/large_workload.txt

# Export results
./cachesim -e results.csv traces/workload.txt
```

#### Configuration File

```json
{
  "l1": {
    "size": 32768,
    "associativity": 4,
    "blockSize": 64,
    "replacementPolicy": "LRU",
    "writePolicy": "WriteBack",
    "prefetch": {
      "enabled": true,
      "distance": 4,
      "adaptive": true
    }
  },
  "l2": {
    "size": 262144,
    "associativity": 8,
    "blockSize": 64,
    "replacementPolicy": "PLRU"
  },
  "victimCache": {
    "enabled": true,
    "size": 4
  },
  "multiprocessor": {
    "numCores": 4,
    "coherence": "MESI",
    "interconnect": "Bus"
  }
}
```

## Advanced Features

### Replacement Policies (v1.1.0+)

Choose the best policy for your workload:

```cpp
// LRU - Best for temporal locality
config.replacementPolicy = ReplacementPolicy::LRU;

// FIFO - Simple, predictable
config.replacementPolicy = ReplacementPolicy::FIFO;

// Random - Good for avoiding pathological cases
config.replacementPolicy = ReplacementPolicy::Random;

// Pseudo-LRU - Approximates LRU with less overhead
config.replacementPolicy = ReplacementPolicy::PLRU;

// NRU - Not Recently Used (v1.2.0)
config.replacementPolicy = ReplacementPolicy::NRU;
```

### Write Policies

```cpp
// Write-back (default) - Better performance
config.writePolicy = WritePolicy::WriteBack;

// Write-through - Simpler, ensures consistency
config.writePolicy = WritePolicy::WriteThrough;

// No-write-allocate (v1.2.0) - Skip allocation on write miss
config.writePolicy = WritePolicy::NoWriteAllocate;
```

### Prefetching Strategies

```cpp
// Enable basic prefetching
config.prefetchEnabled = true;
config.prefetchDistance = 4;

// Stride-based prefetching
config.useStridePrediction = true;
config.strideTableSize = 1024;

// Adaptive prefetching
config.useAdaptivePrefetching = true;
```

### Victim Cache (v1.2.0)

Reduce conflict misses with a victim cache:

```cpp
// Enable 4-entry victim cache
config.useVictimCache = true;
config.victimCacheSize = 4;

// Best for direct-mapped or low-associativity caches
if (config.l1Config.associativity <= 2) {
    config.victimCacheSize = 8;  // Larger victim cache
}
```

## Performance Tuning

### Identifying Bottlenecks

1. **High Miss Rate**
   - Increase cache size
   - Increase associativity
   - Enable prefetching
   - Add victim cache

2. **Many Conflict Misses**
   - Increase associativity
   - Use victim cache
   - Try different replacement policy

3. **Poor Prefetch Accuracy**
   - Adjust prefetch distance
   - Enable adaptive prefetching
   - Analyze access patterns

### Optimization Workflow

```bash
# 1. Baseline measurement
./cachesim -e baseline.csv traces/workload.txt

# 2. Try different configurations
for assoc in 1 2 4 8; do
    ./cachesim 64 32768 $assoc 262144 8 0 0 traces/workload.txt \
        > results_assoc_${assoc}.txt
done

# 3. Compare results
./scripts/compare_results.py results_assoc_*.txt

# 4. Enable advanced features
./cachesim --victim-cache --charts traces/workload.txt
```

### Memory Access Pattern Analysis

```cpp
// Use the profiler to understand your workload
MemoryProfiler profiler;

// Process trace
while (auto access = parser.getNextAccess()) {
    profiler.trackAccess(access->address, access->isWrite);
}

// Analyze patterns
auto pattern = profiler.detectPattern();
switch (pattern) {
    case AccessPattern::Sequential:
        std::cout << "Enable prefetching with distance 4-8\n";
        break;
    case AccessPattern::Strided:
        std::cout << "Use stride prefetcher\n";
        break;
    case AccessPattern::Random:
        std::cout << "Increase cache size or associativity\n";
        break;
}
```

## Multi-Processor Simulation (v1.2.0)

### Basic Multi-Core Setup

```cpp
// Configure 4-core system
MultiProcessorSystem::Config mpConfig;
mpConfig.numProcessors = 4;
mpConfig.l1Config = {32768, 4, 64, false, 0};  // Private L1
mpConfig.sharedL2Config = {1048576, 16, 64, true, 4};  // Shared L2
mpConfig.enableCoherence = true;
mpConfig.interconnectLatency = 10;

MultiProcessorSystem system(mpConfig);
```

### Running Parallel Workloads

```cpp
// Prepare per-core traces
std::vector<std::string> traces = {
    "traces/core0.txt",
    "traces/core1.txt",
    "traces/core2.txt",
    "traces/core3.txt"
};

// Run simulation
uint64_t cycles = system.simulateParallelTraces(traces);

// Analyze results
system.printSystemStats();
```

### Cache Coherence

MESI protocol states:
- **M**odified: Exclusive and dirty
- **E**xclusive: Exclusive and clean
- **S**hared: May be in other caches
- **I**nvalid: Not valid

```cpp
// Example: Producer-consumer pattern
// Core 0 (Producer)
w 0x1000    # Gets M state
w 0x1008    # Stays M

// Core 1 (Consumer)
r 0x1000    # Core 0: M→S, Core 1: I→S
r 0x1008    # Cores share in S state
```

### Synchronization Primitives

```cpp
// Atomic operations
processor->atomicAccess(lockAddr, true);  // Acquire lock

// Memory barriers
processor->memoryBarrier(true, false);   // Acquire barrier
// ... critical section ...
processor->memoryBarrier(false, true);   // Release barrier

// Global barrier (all processors)
system.globalBarrier();
```

## Visualization and Analysis

### Cache State Visualization

```bash
# Enable cache state visualization
./cachesim --visualize --charts traces/workload.txt
```

Output example:
```
╔══════════════════ L1 Cache State ═══════════════════╗
║ Set │ Way │     Tag     │ Valid │ Dirty │   Address   │
╠═════╪═════╪═════════════╪═══════╪═══════╪═════════════╣
║   0 │   0 │ 0x000000100 │  Yes  │  No   │ 0x00001000  │
║   0 │   1 │ 0x000000200 │  Yes  │  Yes  │ 0x00002000  │
║   1 │   0 │ 0x000000140 │  Yes  │  No   │ 0x00001400  │
╚═══════════════════════════════════════════════════════╝
```

### Statistical Charts (v1.2.0)

```cpp
// Generate hit rate comparison
std::vector<std::pair<std::string, double>> hitRates = {
    {"L1 Cache", 0.92},
    {"L2 Cache", 0.85},
    {"Victim Cache", 0.45}
};

std::cout << Visualization::generateHistogram(hitRates, 50, true);

// Memory access heatmap
std::cout << Visualization::generateHeatmap(accessMatrix, 
                                          rowLabels, colLabels);

// Miss rate over time
std::cout << Visualization::generateLineChart(missRateHistory,
                                            80, 20, "Miss Rate Trend");
```

### Performance Reports

```bash
# Generate comprehensive report
./cachesim -e report.csv --verbose traces/workload.txt

# Visualize with external tools
python3 scripts/visualize_results.py report.csv
```

## Best Practices

### 1. Start Simple
Begin with basic configurations before enabling advanced features:
```bash
# Step 1: Baseline
./cachesim 64 32768 1 0 0 0 0 traces/workload.txt

# Step 2: Add associativity
./cachesim 64 32768 4 0 0 0 0 traces/workload.txt

# Step 3: Add L2
./cachesim 64 32768 4 262144 8 0 0 traces/workload.txt

# Step 4: Enable features
./cachesim 64 32768 4 262144 8 1 4 traces/workload.txt
```

### 2. Validate Results
Always sanity-check simulation results:
```cpp
assert(hitRate + missRate == 1.0);
assert(reads + writes == totalAccesses);
assert(l1Misses <= totalAccesses);
```

### 3. Use Appropriate Metrics
- **Hit Rate**: Overall performance indicator
- **MPKI**: Misses Per Kilo Instructions
- **Average Access Time**: Considers all levels
- **Bus Utilization**: For multiprocessor systems

### 4. Consider Workload Characteristics
```cpp
// Sequential workload
if (pattern == Sequential) {
    config.prefetchEnabled = true;
    config.prefetchDistance = 8;
}

// Random workload
if (pattern == Random) {
    config.associativity = 16;  // Reduce conflicts
    config.prefetchEnabled = false;  // Avoid pollution
}
```

### 5. Document Your Experiments
```bash
# Create experiment log
cat > experiment.log << EOF
Date: $(date)
Workload: matrix_multiply.txt
Goal: Reduce L1 miss rate below 5%

Baseline:
- Config: 32KB, 4-way, LRU
- Result: 8.3% miss rate

Experiment 1: Increase size to 64KB
- Result: 5.7% miss rate

Experiment 2: Add victim cache
- Result: 4.8% miss rate ✓
EOF
```

## Troubleshooting

### Common Issues

**Q: Simulation runs very slowly**
- Enable parallel processing: `-p $(nproc)`
- Use release build: `cmake -DCMAKE_BUILD_TYPE=Release`
- Reduce trace size for initial testing

**Q: Unexpected miss rates**
- Verify trace file format
- Check cache size calculations
- Ensure address alignment

**Q: Coherence issues in multiprocessor**
- Check atomic operations usage
- Verify barrier placement
- Enable coherence debugging

### Debug Mode

```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with verbose output
./cachesim --verbose --debug traces/problem.txt

# Use debugger
gdb ./cachesim
(gdb) break Cache::access
(gdb) run traces/problem.txt
```

## Next Steps

1. Explore the [examples/](../examples/) directory
2. Read the [design documentation](design.md)
3. Try the performance challenges in [benchmarks/](../benchmarks/)
4. Contribute your improvements!

Happy simulating! 🚀
