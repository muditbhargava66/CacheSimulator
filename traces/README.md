# Cache Simulator Trace Files

This directory contains various memory trace files for testing and benchmarking the cache simulator.

## Trace File Categories

### Basic Traces
- **trace1.txt** - Simple sequential access pattern (1000 accesses)
- **trace2.txt** - Random access pattern with moderate locality (2000 accesses)
- **trace3.txt** - Mixed read/write workload (1500 accesses)
- **trace4.txt** - Strided access pattern (1200 accesses)

### Advanced Feature Traces
- **advanced_features.txt** - Demonstrates all advanced cache features (5000 accesses)
- **victim_cache_test.txt** - Optimized for victim cache testing (3000 accesses)
- **multiprocessor_coherence.txt** - Multi-processor cache coherence scenarios (4000 accesses)

### Workload-Specific Traces
- **comprehensive_test.txt** - Complete feature validation trace (10000 accesses)
- **conflict_heavy.txt** - High conflict miss scenarios (2500 accesses)
- **matrix_multiply.txt** - Matrix multiplication access pattern (8000 accesses)
- **mixed_locality.txt** - Mixed temporal and spatial locality (6000 accesses)
- **producer_consumer.txt** - Producer-consumer synchronization pattern (3500 accesses)
- **streaming_workload.txt** - Sequential streaming access (7000 accesses)
- **write_intensive.txt** - Write-heavy workload (4500 accesses)
- **write_policy_test.txt** - Write policy validation (2000 accesses)

## Trace File Format

### Standard Format
```
# Comments start with #
R 0x1000    # Read from address 0x1000
W 0x1004    # Write to address 0x1004
```

### JSON Format
```json
{
  "accesses": [
    {"address": "0x1000", "type": "read"},
    {"address": "0x1004", "type": "write"}
  ]
}
```

## Usage Examples

### Basic Simulation
```bash
./bin/cachesim traces/trace1.txt
```

### Advanced Features
```bash
./bin/cachesim --victim-cache --parallel traces/advanced_features.txt
```

### Multi-processor Simulation
```bash
./bin/cachesim --config configs/multiprocessor.json traces/multiprocessor_coherence.txt
```

### Performance Benchmarking
```bash
./bin/cachesim --benchmark traces/comprehensive_test.txt
```

## Creating Custom Traces

Use the trace generator tool:
```bash
# Generate sequential trace
./bin/tools/trace_generator --pattern sequential --num 10000 --output custom_sequential.txt

# Generate random trace
./bin/tools/trace_generator --pattern random --num 5000 --write 0.3 --output custom_random.txt

# Generate strided trace
./bin/tools/trace_generator --pattern strided --stride 128 --num 8000 --output custom_strided.txt

# Generate looping trace
./bin/tools/trace_generator --pattern looping --loop-size 20 --repetitions 10 --output custom_loop.txt

# Generate mixed locality trace
./bin/tools/trace_generator --pattern mixed --regions 5 --locality 0.8 --num 15000 --output custom_mixed.txt

# Generate all standard patterns
./bin/tools/trace_generator --generate-all custom_traces/
```

Available patterns:
- `sequential` - Sequential memory access
- `random` - Random memory access  
- `strided` - Strided access pattern
- `looping` - Repeating loop pattern
- `mixed` - Mixed pattern with locality

## Trace Analysis

Analyze trace characteristics:
```bash
# Basic analysis
./bin/tools/cache_analyzer traces/matrix_multiply.txt

# Detailed analysis with recommendations
./bin/tools/cache_analyzer --recommend traces/mixed_locality.txt

# Export analysis to CSV
./bin/tools/cache_analyzer --export analysis.csv traces/comprehensive_test.txt
```

## Batch Processing

Process multiple traces:
```bash
# Run benchmarks on all traces
./scripts/run_benchmarks.sh

# Compare configurations across traces
./bin/tools/performance_comparison --configs configs/high_performance.json,configs/victim_cache_config.json traces/*.txt
```