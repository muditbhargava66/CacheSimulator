# Multi-processor Simulation

The Cache Simulator v1.2.0 includes comprehensive multi-processor simulation capabilities with cache coherence protocols.

## Overview

The multi-processor simulation framework supports:
- Multiple processor cores with private L1 caches
- Cache coherence protocols (MESI)
- Various interconnect topologies
- Atomic operations and memory barriers
- Comprehensive statistics collection

## Configuration

### Basic Multi-processor Setup
```json
{
  "multiprocessor": {
    "enabled": true,
    "numProcessors": 4,
    "coherenceProtocol": "MESI",
    "interconnect": "Bus",
    "interconnectLatency": 10
  },
  "l1": {
    "size": 32768,
    "associativity": 4,
    "blockSize": 64,
    "replacementPolicy": "NRU"
  }
}
```

### Advanced Configuration
```json
{
  "multiprocessor": {
    "enabled": true,
    "numProcessors": 8,
    "coherenceProtocol": "MESI",
    "interconnect": "Mesh",
    "interconnectLatency": 5,
    "directoryBased": true,
    "atomicOperations": true
  }
}
```

## Coherence Protocols

### MESI Protocol
The Modified, Exclusive, Shared, Invalid protocol ensures cache coherence:

- **Modified (M)**: Block is modified and exclusive to this cache
- **Exclusive (E)**: Block is unmodified and exclusive to this cache
- **Shared (S)**: Block is unmodified and may be in other caches
- **Invalid (I)**: Block is not valid in this cache

### State Transitions
```
Read Hit:   M→M, E→E, S→S
Read Miss:  I→S/E (depending on sharing)
Write Hit:  M→M, E→M, S→M (invalidate others)
Write Miss: I→M (invalidate others)
```

## Interconnect Topologies

### Bus Interconnect
- **Description**: Shared bus connecting all processors
- **Latency**: Uniform for all communications
- **Scalability**: Limited by bus bandwidth
- **Best for**: Small systems (2-4 processors)

### Crossbar Interconnect
- **Description**: Full crossbar switch
- **Latency**: Point-to-point connections
- **Scalability**: Good but expensive
- **Best for**: Medium systems (4-16 processors)

### Mesh Interconnect
- **Description**: 2D mesh topology
- **Latency**: Distance-dependent
- **Scalability**: Excellent
- **Best for**: Large systems (16+ processors)

## Simulation Features

### Atomic Operations
```cpp
// Supported atomic operations
processor->atomicAccess(address, true);   // Atomic read-modify-write
processor->compareAndSwap(address, old, new);
processor->fetchAndAdd(address, value);
```

### Memory Barriers
```cpp
// Memory ordering constraints
processor->memoryBarrier(true, false);   // Acquire barrier
processor->memoryBarrier(false, true);   // Release barrier
processor->memoryBarrier(true, true);    // Full barrier
```

### Synchronization Primitives
```cpp
// Global synchronization
system.globalBarrier();                  // Wait for all processors
system.globalFence();                    // Memory fence across all processors
```

## Trace Format for Multi-processor

### Per-processor Traces
```
# Processor 0 trace (proc0.trace)
R 0x1000    # Private access
W 0x2000    # Shared access
A 0x3000    # Atomic operation
B           # Memory barrier

# Processor 1 trace (proc1.trace)
R 0x2000    # Shared access (coherence required)
W 0x4000    # Private access
```

### Unified Trace Format
```
# Unified trace with processor IDs
P0 R 0x1000
P1 R 0x2000
P0 W 0x2000    # Coherence event
P1 A 0x3000    # Atomic operation
```

## Statistics and Analysis

### Coherence Statistics
- Cache-to-cache transfers
- Invalidation messages
- Directory lookups
- Protocol state transitions

### Interconnect Statistics
- Message counts by type
- Average message latency
- Bandwidth utilization
- Congestion events

### Performance Metrics
- Coherence miss rate
- Synchronization overhead
- Scalability analysis
- Load balancing metrics

## Example Usage

### Command Line
```bash
# Run multi-processor simulation
./bin/cachesim --config mp_config.json --parallel 4 traces/mp_trace.txt

# With visualization
./bin/cachesim --config mp_config.json --visualize --charts traces/mp_trace.txt
```

### Programmatic Usage
```cpp
// Create multi-processor system
MultiProcessorSystem::Config config;
config.numProcessors = 4;
config.coherenceProtocol = CoherenceProtocol::MESI;
config.interconnect = InterconnectType::Mesh;

MultiProcessorSystem system(config);

// Run simulation
std::vector<std::string> traceFiles = {"p0.trace", "p1.trace", "p2.trace", "p3.trace"};
uint64_t cycles = system.simulateParallelTraces(traceFiles);

// Get statistics
auto stats = system.getSystemStats();
```

## Performance Considerations

### Scalability
- Bus: Up to 4 processors efficiently
- Crossbar: Up to 16 processors
- Mesh: 64+ processors supported

### Memory Requirements
- Base system: ~100MB
- Per processor: ~50MB additional
- Scales linearly with cache sizes

### Simulation Speed
- Single processor: 1M accesses/second
- 4 processors: 800K accesses/second
- 8 processors: 600K accesses/second
- Parallel simulation available for large traces

## Validation

The multi-processor simulation has been validated against:
- Known coherence protocol behaviors
- Published benchmark results
- Hardware performance counters
- Academic simulation frameworks

## Limitations

Current limitations:
- MESI protocol only (MSI, MOESI planned)
- Directory-based coherence (snooping planned)
- Limited atomic operation set
- No NUMA modeling (planned for v1.3.0)

## Future Enhancements

Planned for future versions:
- Additional coherence protocols
- NUMA topology support
- Advanced interconnect models
- Power modeling for multi-processor systems
- GPU coherence integration