# Performance Analysis Tools

This guide covers the performance analysis and profiling tools available in the Cache Simulator.

## Overview

The Cache Simulator provides several analysis capabilities:

- Cache hit/miss analysis
- Working set analysis
- Reuse distance calculation
- Access pattern classification
- Configuration comparison

## Cache Analyzer

### Basic Analysis

```bash
# Run basic analysis
./cachesim --vis traces/workload.txt
```

Output includes:

- L1/L2 hit rates
- Miss type breakdown (compulsory, capacity, conflict)
- Access pattern statistics

### Detailed Statistics

```bash
# Enable detailed statistics
./cachesim --verbose traces/workload.txt
```

### Export Results

```bash
# Export to CSV
./cachesim --export results.csv traces/workload.txt
```

## Performance Comparison

### Comparing Configurations

Create multiple configuration files and compare results:

```bash
# Compare two configurations
./cachesim --config config1.json traces/workload.txt > result1.txt
./cachesim --config config2.json traces/workload.txt > result2.txt
```

### Benchmarking

```bash
# Run benchmark mode
./cachesim --benchmark traces/workload.txt
```

Benchmark mode provides:

- Processing time metrics
- Throughput (accesses/second)
- Memory usage statistics

## Visualization

### Cache State Visualization

```bash
# Enable visualization
./cachesim --vis traces/workload.txt
```

Displays ASCII tables showing:

- Cache block states
- Tag values
- Valid/dirty status
- Access counts

### Statistical Charts

The visualization output includes:

- Access distribution histogram
- Memory address heatmap
- Hit rate over time

## Power Analysis (v1.3.0)

### Enable Power Modeling

```bash
# Default 45nm technology
./cachesim --power traces/workload.txt

# Specify technology node
./cachesim --power --tech-node 7 traces/workload.txt
```

### Power Metrics

| Metric | Description | Unit |
|--------|-------------|------|
| Dynamic Energy | Energy per access | pJ |
| Total Energy | Cumulative energy | nJ |
| Leakage Power | Static power consumption | mW |
| EDP | Energy-Delay Product | pJ*ns |

### Technology Nodes

Supported nodes: 7nm, 14nm, 22nm, 32nm, 45nm

## Analysis Workflow

### Step 1: Profile Workload

```bash
./cachesim --verbose traces/workload.txt
```

Review:
- Miss rate distribution
- Access patterns
- Working set size

### Step 2: Identify Bottlenecks

Look for:
- High conflict miss rate -> Consider victim cache
- High capacity miss rate -> Increase cache size
- Low reuse -> Check prefetching settings

### Step 3: Test Configurations

```bash
# Test with victim cache
./cachesim --victim-cache traces/workload.txt

# Test different replacement policies
./cachesim --config lru_config.json traces/workload.txt
./cachesim --config nru_config.json traces/workload.txt
```

### Step 4: Compare Results

Compare hit rates, access times, and power consumption to find optimal configuration.

## Interpreting Results

### Hit Rate Analysis

| Hit Rate | Interpretation |
|----------|----------------|
| > 95% | Excellent - cache is well-sized |
| 85-95% | Good - typical for most workloads |
| 70-85% | Moderate - consider optimizations |
| < 70% | Poor - investigate miss patterns |

### Miss Type Breakdown

- **Compulsory**: Unavoidable on first access
- **Capacity**: Cache too small for working set
- **Conflict**: Set conflicts due to low associativity

### Optimization Recommendations

| Miss Type | High Percentage | Recommendation |
|-----------|-----------------|----------------|
| Compulsory | > 50% | Enable prefetching |
| Capacity | > 40% | Increase cache size |
| Conflict | > 30% | Add victim cache or increase associativity |

## Parallel Processing

For large trace files:

```bash
# Enable parallel processing
./cachesim --parallel 4 traces/large_workload.txt
```

Performance scaling:
- 2 threads: ~1.8x speedup
- 4 threads: ~3.2x speedup  
- 8 threads: ~5.5x speedup

## Best Practices

1. **Start with default configuration** to establish baseline
2. **Profile before optimizing** to identify bottlenecks
3. **Change one parameter at a time** for clear comparisons
4. **Use representative workloads** for accurate analysis
5. **Consider power/performance tradeoffs** with power analysis

## See Also

- [Configuration](configuration.md) - Configuration options
- [Power Modeling](../features/power-modeling.md) - Power analysis details
- [Examples](examples.md) - More usage examples
