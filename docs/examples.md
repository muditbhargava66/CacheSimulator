# Cache Simulator Examples and Case Studies

This document provides practical examples and case studies for using the Cache Simulator. These examples demonstrate how to configure the simulator for various scenarios and how to interpret the results.

## Table of Contents

1. [Basic Usage](#basic-usage)
2. [Analysis Examples](#analysis-examples)
   - [Sequential Access Pattern](#sequential-access-pattern)
   - [Random Access Pattern](#random-access-pattern)
   - [Mixed Access Pattern](#mixed-access-pattern)
3. [Case Studies](#case-studies)
   - [Optimizing Cache Size](#case-study-1-optimizing-cache-size)
   - [Evaluating Prefetching Effectiveness](#case-study-2-evaluating-prefetching-effectiveness)
   - [Cache Coherence Analysis](#case-study-3-cache-coherence-analysis)
4. [Advanced Configurations](#advanced-configurations)
   - [Multi-level Cache Hierarchy](#multi-level-cache-hierarchy)
   - [Advanced Prefetching](#advanced-prefetching)
   - [Replacement Policy Comparison](#replacement-policy-comparison)
5. [Trace File Examples](#trace-file-examples)

## Basic Usage

### Running a Simple Simulation

To run a basic simulation with the Cache Simulator:

```bash
./build/bin/cachesim 64 32768 4 262144 8 0 0 traces/trace1.txt
```

This command configures:
- 64-byte cache blocks
- 32KB L1 cache (32768 bytes) with 4-way associativity
- 256KB L2 cache (262144 bytes) with 8-way associativity
- No prefetching (0 0)
- Using trace1.txt as the memory access trace

### Interpreting Results

The simulator outputs statistics like:

```
L1 reads: 1500
L1 writes: 500
L1 misses: 200
L1 hits: 1800
L1 miss rate: 10.0%
...
```

Key metrics to look for:
- **Miss rate**: Lower is better, indicates cache effectiveness
- **Hit rate**: Higher is better, complementary to miss rate
- **Write-backs**: Indicates dirty evictions to lower level
- **Prefetch accuracy**: For configurations with prefetching, indicates how useful prefetches were

## Analysis Examples

### Sequential Access Pattern

Sequential access patterns benefit significantly from prefetching. Compare these commands:

```bash
# Without prefetching
./build/bin/cachesim 64 16384 2 65536 4 0 0 traces/trace2.txt

# With sequential prefetching
./build/bin/cachesim 64 16384 2 65536 4 1 4 traces/trace2.txt

# With adaptive prefetching
./build/bin/cachesim 64 16384 2 65536 4 1 8 traces/trace2.txt
```

Expected results:
- Without prefetching: ~40-50% miss rate
- With sequential prefetching: ~10-20% miss rate
- With adaptive prefetching: ~10-15% miss rate

Takeaway: Prefetching provides substantial benefits for sequential patterns, with adaptive prefetching performing slightly better than fixed-distance prefetching.

### Random Access Pattern

Random access patterns typically don't benefit from simple prefetching:

```bash
# Run with and without prefetching on random trace
./build/bin/cachesim 64 32768 4 262144 8 0 0 traces/random_trace.txt
./build/bin/cachesim 64 32768 4 262144 8 1 4 traces/random_trace.txt
```

Expected results:
- Without prefetching: ~80-90% miss rate
- With prefetching: ~80-90% miss rate, possibly worse due to useless prefetches

Takeaway: For random access patterns, increasing cache size and associativity is more effective than prefetching.

### Mixed Access Pattern

For mixed access patterns, adaptive prefetching can be beneficial:

```bash
# Generate a mixed pattern trace
./build/bin/tools/trace_generator -p mixed -o mixed_trace.txt

# Run with different configurations
./build/bin/cachesim 64 32768 4 262144 8 0 0 mixed_trace.txt
./build/bin/cachesim 64 32768 4 262144 8 1 4 mixed_trace.txt
```

Analyze the results to see how different configurations handle mixed patterns.

## Case Studies

### Case Study 1: Optimizing Cache Size

This case study explores the trade-off between cache size and performance:

```bash
# Script to run multiple cache sizes
for L1_SIZE in 8192 16384 32768 65536 131072; do
  for L2_SIZE in 65536 131072 262144 524288 1048576; do
    ./build/bin/cachesim 64 $L1_SIZE 4 $L2_SIZE 8 0 0 traces/trace3.txt >> cache_size_results.txt
  done
done
```

Analyze results to find the optimal cache size beyond which returns diminish. 

Typical findings:
- Doubling L1 from 16KB to 32KB might reduce miss rate by 30%
- Doubling L1 from 32KB to 64KB might only reduce miss rate by 10%
- L2 increases show diminishing returns after 256KB for many workloads

### Case Study 2: Evaluating Prefetching Effectiveness

This case study compares different prefetching strategies:

```bash
# Without prefetching (baseline)
./build/bin/cachesim 64 32768 4 262144 8 0 0 traces/trace4.txt

# Simple sequential prefetching (distance 4)
./build/bin/cachesim 64 32768 4 262144 8 1 4 traces/trace4.txt

# Aggressive sequential prefetching (distance 8)
./build/bin/cachesim 64 32768 4 262144 8 1 8 traces/trace4.txt

# Adaptive prefetching
./build/bin/cachesim 64 32768 4 262144 8 1 16 traces/trace4.txt
```

With trace4.txt (mixed access patterns), you might see:
- Sequential prefetching (distance 4): 15% improvement over baseline
- Aggressive prefetching (distance 8): 20% improvement but more wasted bandwidth
- Adaptive prefetching: 25% improvement with similar bandwidth usage to distance 4

### Case Study 3: Cache Coherence Analysis

For analyzing cache coherence behavior with the MESI protocol:

```bash
# Generate a trace with coherence traffic
./build/bin/tools/trace_generator --coherence -o coherence_trace.txt

# Run the simulation with MESI statistics enabled
./build/bin/cachesim 64 32768 4 262144 8 0 0 coherence_trace.txt --verbose
```

Look for MESI state transitions in the output:
- Transitions between M->S states indicate sharing activity
- High I->M transitions indicate write misses
- High S->M transitions indicate write hits to shared blocks

## Advanced Configurations

### Multi-level Cache Hierarchy

To simulate a three-level cache hierarchy (requires custom build or future version):

```bash
# Example with L1, L2, and L3 (conceptual - actual syntax depends on implementation)
./build/bin/cachesim --l1 32768:4:64 --l2 262144:8:64 --l3 8388608:16:64 traces/trace1.txt
```

### Advanced Prefetching

To use stride-based and pattern-based prefetching:

```bash
# Enable stride prediction
./build/bin/cachesim 64 32768 4 262144 8 1 4 --stride-prediction traces/trace3.txt

# Enable adaptive prefetching
./build/bin/cachesim 64 32768 4 262144 8 1 8 --adaptive-prefetch traces/trace3.txt
```

### Replacement Policy Comparison

To compare different replacement policies:

```bash
# Default LRU policy
./build/bin/cachesim 64 32768 4 262144 8 0 0 --replacement=lru traces/trace1.txt

# FIFO policy (if implemented)
./build/bin/cachesim 64 32768 4 262144 8 0 0 --replacement=fifo traces/trace1.txt

# Random policy (if implemented)
./build/bin/cachesim 64 32768 4 262144 8 0 0 --replacement=random traces/trace1.txt
```

## Trace File Examples

Here are some examples of trace file formats:

### Simple Read/Write Format

```
r 0x1000
w 0x2000
r 0x1040
w 0x2040
r 0x1080
```

### Advanced Format with PC and Thread ID

```
# format: operation address [pc] [thread_id]
r 0x1000 0x40000 1
w 0x2000 0x40008 1
r 0x3000 0x40010 2
```

### Creating Custom Traces

You can create custom traces with the trace generator:

```bash
# Create a sequential access pattern
./build/bin/tools/trace_generator -p sequential -n 1000 -o sequential.txt

# Create a random access pattern
./build/bin/tools/trace_generator -p random -n 1000 -o random.txt

# Create a mixed access pattern with 30% writes
./build/bin/tools/trace_generator -p mixed -n 1000 -w 0.3 -o mixed.txt
```

For more information about trace file formats and generation, see the [Trace File Format](#) documentation.

---

## Comparative Analysis Example

Below is an example of comparing multiple configurations and visualizing the results using the included scripts:

```bash
# Run benchmark script
./scripts/run_simulations.sh

# Analyze results
cat results/benchmark_results.csv

# If gnuplot is available, visualize results
gnuplot -e "set terminal png; set output 'miss_rate_comparison.png'; \
            set style data histogram; \
            set style histogram cluster gap 1; \
            set style fill solid border -1; \
            set title 'Miss Rate Comparison'; \
            set ylabel 'Miss Rate (%)'; \
            set yrange [0:100]; \
            plot 'results/benchmark_results.csv' using 13:xtic(2) title 'Miss Rate'"
```

This will generate a histogram comparing miss rates across different configurations.