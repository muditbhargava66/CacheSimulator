# Cache Simulator Design Documentation

## Table of Contents
1. [Introduction](#introduction)
2. [Architecture Overview](#architecture-overview)
3. [Core Components](#core-components)
   - [CacheBlock & CacheSet](#cacheblock--cacheset)
   - [Cache](#cache)
   - [MemoryHierarchy](#memoryhierarchy)
   - [StreamBuffer](#streambuffer)
   - [StridePredictor](#stridepredictor)
   - [AdaptivePrefetcher](#adaptiveprefetcher)
   - [MESI Protocol](#mesi-protocol)
4. [Utility Components](#utility-components)
   - [TraceParser](#traceparser)
   - [Statistics](#statistics)
   - [Visualization](#visualization)
   - [Configuration](#configuration)
   - [Logging](#logging)
   - [Benchmarking](#benchmarking)
   - [Memory Profiler](#memory-profiler)
5. [Algorithms & Policies](#algorithms--policies)
   - [Replacement Policies](#replacement-policies)
   - [Prefetching Algorithms](#prefetching-algorithms)
   - [Cache Coherence](#cache-coherence)
6. [Implementation Details](#implementation-details)
   - [C++17 Features](#c17-features)
   - [Performance Optimizations](#performance-optimizations)
   - [Code Organization](#code-organization)
7. [Testing Strategy](#testing-strategy)
8. [Future Extensions](#future-extensions)

## Introduction

The Cache Simulator is a comprehensive simulation tool for studying memory hierarchy behavior in computer systems. It models a two-level cache hierarchy with configurable parameters and advanced features like prefetching, cache coherence, and detailed statistics tracking. The simulator is designed to be highly customizable, allowing researchers and students to experiment with different cache configurations and analyze their performance.

The simulator takes as input a memory access trace that describes a sequence of read and write operations with their corresponding memory addresses. It then simulates how these accesses would flow through the cache hierarchy, tracking hits, misses, replacements, and other events. Various statistics are collected during the simulation, which can be used to evaluate the effectiveness of different cache designs and policies.

This document describes the design of the Cache Simulator, including its architecture, components, algorithms, and implementation details.

## Architecture Overview

The Cache Simulator employs a modular, layered architecture that separates core cache functionality from utilities and user interface components:

```
┌──────────────────────────────────────────────────┐
│                   Applications                   │
│ (Main Program, Tests, Trace Generator, Scripts)  │
└───────────────────────┬──────────────────────────┘
                        │
┌───────────────────────▼──────────────────────────┐
│               Memory Hierarchy API               │
│     (MemoryHierarchy, Configuration, Stats)      │
└───────────────────────┬──────────────────────────┘
                        │
┌───────────────────────▼──────────────────────────┐
│                Core Components                   │
│  (Cache, StreamBuffer, Prefetchers, MESI, etc.)  │
└───────────────────────┬──────────────────────────┘
                        │
┌───────────────────────▼──────────────────────────┐
│             Utility Components                   │
│   (Trace Parsing, Logging, Visualization, etc.)  │
└──────────────────────────────────────────────────┘
```

The design follows several key architectural principles:

1. **Separation of Concerns**: Each component has a focused responsibility.
2. **Modularity**: Components can be developed, tested, and replaced independently.
3. **Extensibility**: New policies, algorithms, and features can be easily added.
4. **Configurability**: Extensive runtime customization options are available.
5. **Modern C++ Design**: Leveraging C++17 features for safer, more expressive code.

## Core Components

### CacheBlock & CacheSet

The foundational data structures in the Cache Simulator are `CacheBlock` and `CacheSet`:

- `CacheBlock` represents a single block in the cache, containing:
  - Valid and dirty flags
  - Tag bits for address matching
  - MESI state for cache coherence
  - Optional data storage (for functional simulation)

- `CacheSet` represents a set of blocks in a set-associative cache:
  - Vector of CacheBlocks
  - LRU ordering information for replacement

These structures are designed to be memory-efficient while still providing all necessary state information for accurate simulation.

### Cache

The `Cache` class is a central component representing a single cache level (L1 or L2). It manages:

- Cache organization (size, associativity, block size)
- Block lookup and placement
- Replacement policy implementation
- Prefetching integration
- Hit/miss statistics collection
- MESI protocol state transitions

The `Cache` class provides a clean `access()` method that handles all the complex operations of a cache access while maintaining detailed statistics.

### MemoryHierarchy

The `MemoryHierarchy` class orchestrates the entire cache hierarchy, managing:

- Multiple cache levels (L1, L2)
- Memory access flow through the hierarchy
- Prefetching configuration and coordination
- Statistics aggregation
- Trace file processing
- Performance metrics calculation

This class serves as the main API for applications interacting with the simulator, providing methods to access memory, process traces, and retrieve statistics.

### StreamBuffer

The `StreamBuffer` class implements a simple hardware prefetching mechanism:

- Prefetches sequential blocks following a cache miss
- Maintains a buffer of prefetched addresses
- Tracks hits in the buffer
- Shifts out consumed entries
- Provides statistics on buffer effectiveness

Stream buffers are a simple but effective prefetching technique that works well for sequential access patterns.

### StridePredictor

The `StridePredictor` class implements a more sophisticated prefetching mechanism:

- Detects constant stride patterns in memory accesses
- Maintains a table of recent addresses and strides
- Predicts future accesses based on detected strides
- Provides confidence levels for predictions
- Tracks prediction accuracy statistics

Stride prediction is effective for regular access patterns with non-unit strides, such as array traversals in multidimensional arrays.

### AdaptivePrefetcher

The `AdaptivePrefetcher` class implements an advanced prefetching mechanism:

- Dynamically selects between prefetching strategies
- Adjusts prefetch distance based on effectiveness
- Tracks different metrics to guide adaptation
- Supports multiple concurrently active strategies
- Maintains detailed effectiveness statistics

Adaptive prefetching can deliver good performance across a wider range of access patterns compared to fixed-strategy prefetchers.

### MESI Protocol

The `MESIProtocol` class implements the Modified-Exclusive-Shared-Invalid cache coherence protocol:

- Manages state transitions for cache blocks
- Handles local reads and writes
- Processes remote cache operations
- Coordinates writebacks for modified blocks
- Tracks protocol state transition statistics

The MESI protocol ensures coherent memory views in systems with multiple caches accessing shared memory.

## Utility Components

### TraceParser

The `TraceParser` class handles trace file input:

- Parses different trace file formats
- Validates memory access operations
- Provides detailed error information
- Offers both single-operation and bulk parsing
- Uses efficient, streaming file reading

This component decouples trace file handling from the core simulation logic.

### Statistics

The `Statistics` class manages simulation metrics:

- Collects and organizes various numerical metrics
- Calculates derived statistics
- Provides data export capabilities
- Supports metric comparison between runs
- Offers customizable reporting formats

This utility enables detailed analysis of simulation results.

### Visualization

The `Visualization` class provides text-based visualization:

- Generates ASCII visualizations of cache state
- Renders histograms and heatmaps of memory access patterns
- Supports colorized output where available
- Exports data for external visualization tools
- Provides progress indicators for long simulations

These visualizations aid in understanding simulator behavior.

### Configuration

The `ConfigManager` class manages configuration parameters:

- Loads configurations from files (JSON, INI)
- Parses command-line arguments
- Validates configuration parameters
- Generates parameter sweeps for benchmarking
- Supports configuration comparison

This utility simplifies experimentation with different simulator settings.

### Logging

The `Logger` class provides diagnostic information:

- Supports multiple log levels
- Offers both file and console output
- Provides thread-safe logging
- Allows component-specific named loggers
- Enables selectively verbose output

Logging helps diagnose issues and understand simulator behavior.

### Benchmarking

The `Benchmark` class supports performance measurement:

- Times the execution of simulation functions
- Compares multiple configurations
- Calculates statistical measures of performance
- Provides formatted benchmark reports
- Supports repeated runs for variance analysis

This utility is useful for simulator development and optimization.

### Memory Profiler

The `MemoryProfiler` class analyzes memory access patterns:

- Tracks memory access frequencies
- Detects common access patterns
- Profiles locality characteristics
- Identifies hot regions in memory
- Provides insights for optimization

This tool helps understand workload behavior and aids in prefetcher design.

## Algorithms & Policies

### Replacement Policies

The simulator implements several cache block replacement policies:

1. **LRU (Least Recently Used)**: Replaces the block that has not been accessed for the longest time. This is the default policy and is implemented using an ordering vector in each cache set.

2. **Pseudo-LRU**: A tree-based approximation of LRU that requires less state information but still provides good performance.

3. **Random**: Randomly selects a block for replacement, which can be effective for certain workloads and eliminates pathological cases that affect deterministic policies.

4. **FIFO (First-In-First-Out)**: Replaces the oldest block regardless of access frequency.

5. **NRU (Not Recently Used)**: A simple approximation of LRU using a single bit per block to indicate recent usage.

The policy can be configured on a per-cache basis.

### Prefetching Algorithms

The simulator supports several prefetching algorithms:

1. **Sequential Prefetching**: Prefetches a configurable number of sequential blocks after a miss. Implemented by the `StreamBuffer` class.

2. **Stride Prefetching**: Detects regular strides in access patterns and prefetches accordingly. Implemented by the `StridePredictor` class.

3. **Adaptive Prefetching**: Dynamically adjusts prefetching strategy and aggressiveness based on effectiveness. Implemented by the `AdaptivePrefetcher` class.

4. **Next-N-Line Prefetching**: A simple variation that prefetches the next N blocks on any miss.

5. **PC-based Prefetching**: Uses program counter information (if available in the trace) to predict access patterns specific to different code regions.

These algorithms can be combined in various ways and customized with different parameters.

### Cache Coherence

The MESI protocol implements cache coherence:

1. **Modified (M)**: The block is present only in the current cache and has been modified. It must be written back to memory when evicted.

2. **Exclusive (E)**: The block is present only in the current cache and matches memory.

3. **Shared (S)**: The block may be present in other caches and is clean.

4. **Invalid (I)**: The block is not present or valid in the cache.

The protocol handles transitions between these states based on local and remote operations, ensuring coherent memory views across caches.

## Implementation Details

### C++17 Features

The simulator leverages several C++17 features:

1. **std::optional**: Used for representing potentially missing values, such as nullable parameters and results.

2. **std::variant**: Used for type-safe unions, particularly in return values that can represent different outcomes.

3. **std::string_view**: Used for non-owning references to strings, improving performance in text processing.

4. **std::filesystem**: Used for portable file operations in trace handling and result saving.

5. **Structured Bindings**: Used to simplify code that returns multiple values, like tag and set index extraction.

6. **if constexpr**: Used for compile-time conditional code, improving performance in template functions.

7. **Inline Variables**: Used for cleaner declaration of constants and static members.

8. **[[nodiscard]]**: Used to ensure return values are properly handled for functions that shouldn't be called for side effects.

9. **constexpr if**: Used for compile-time conditional compilation without macros.

10. **auto return type deduction**: Simplifies function declarations while maintaining type safety.

These features improve code safety, clarity, and performance.

### Performance Optimizations

Several optimizations are employed:

1. **Memory Pool Allocation**: Cache blocks are allocated from a memory pool to reduce dynamic allocation overhead.

2. **Bit Manipulation**: Fast bit operations are used for address decomposition and tag comparison.

3. **Vectorized Processing**: Common operations are vectorized when processing multiple blocks.

4. **Cache-Friendly Data Structures**: Data is organized to maximize spatial locality.

5. **Custom Hash Functions**: Optimized hash functions for address lookup.

6. **String Interning**: Reuse of common strings to reduce memory usage.

7. **Streaming I/O**: Efficient trace file reading without loading entire files.

8. **Compiler Hints**: Strategic use of likely/unlikely hints for branch prediction.

These optimizations significantly improve simulation speed, especially for large traces.

### Code Organization

The code is organized following modern C++ practices:

1. **Namespace Usage**: All components are within the `cachesim` namespace to avoid pollution.

2. **Header-Only When Appropriate**: Small utilities are implemented as header-only for simplicity.

3. **Separation of Interface and Implementation**: Larger components separate declaration and definition.

4. **PIMPL Pattern**: Used for components with complex internal details to improve compilation speed.

5. **Consistent Naming**: Camel case for class names, snake case for methods and variables.

6. **Const Correctness**: Rigorous use of const for parameters and methods.

7. **Forward Declarations**: Used to minimize include dependencies.

8. **Explicit Memory Management**: Clear ownership semantics with smart pointers.

9. **Modular Design**: Each component has a single responsibility.

10. **Documentation**: Comprehensive comments for all public interfaces.

This organization enhances maintainability, readability, and compile times.

## Testing Strategy

The simulator employs a multi-level testing approach:

1. **Unit Tests**: Verify individual components in isolation.
   - Test each cache operation separately
   - Verify replacement policy behavior
   - Validate prefetcher predictions
   - Check MESI state transitions

2. **Integration Tests**: Verify interactions between components.
   - Test L1-L2 interactions
   - Verify prefetcher-cache coordination
   - Check coherence protocol across caches

3. **Validation Tests**: Verify the simulator against known outcomes.
   - Test with hand-crafted traces with predictable results
   - Compare against simplified analytical models
   - Validate against academic benchmark results

4. **Property-Based Tests**: Verify invariants and properties.
   - Ensure cache coherence is maintained
   - Verify inclusion properties when applicable
   - Check conservation of blocks

5. **Performance Tests**: Measure and optimize performance.
   - Benchmark with large traces
   - Profile memory usage
   - Analyze scalability with increasing parameters

This comprehensive testing strategy ensures correctness and robustness.

## Future Extensions

Planned extensions include:

1. **Additional Cache Levels**: Support for L3 cache and beyond.

2. **Victim Cache**: Implementation of a small fully-associative victim cache.

3. **Non-Inclusive Policies**: Support for non-inclusive and exclusive cache hierarchies.

4. **Replacement Policy Framework**: Pluggable framework for custom replacement policies.

5. **Advanced Prefetching Algorithms**: Implementation of more sophisticated prefetchers like GHB and Markov predictors.

6. **GPU Cache Modeling**: Extensions for modeling GPU-specific cache architectures.

7. **Multi-Core Simulation**: Support for simulating multiple cores with shared caches.

8. **Graphical Interface**: Development of a GUI for visualization and configuration.

9. **Dynamic Trace Generation**: Integration with instruction-level simulators.

10. **Machine Learning Integration**: Using ML techniques for prefetching and replacement.

These extensions will enhance the simulator's capabilities and keep it relevant for future research.