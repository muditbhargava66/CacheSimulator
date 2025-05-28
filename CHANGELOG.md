# Changelog

All notable changes to the Cache Simulator project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.0] - 2025-05-29

### Added
- **NRU (Not Recently Used) Replacement Policy**: Complete implementation with reference bit tracking
- **No-Write-Allocate Policy**: Support for both write-through and write-back variants
- **Write Combining Buffer**: Coalescing writes to improve memory bandwidth utilization
- **Victim Cache**: Fully associative cache for storing evicted blocks, reducing conflict misses
- **Parallel Processing**: Thread pool and parallel trace processor for multi-core simulation
- **Statistical Charting**: ASCII line charts, pie charts, and scatter plots for data visualization
- **Enhanced Write Policy Framework**: Pluggable system supporting multiple allocation strategies
- **Parallel Benchmarking**: Compare multiple configurations simultaneously

### Changed
- Refactored write policy system to use inheritance-based design
- Updated version to 1.2.0 across all files
- Enhanced visualization with new chart types
- Improved command-line interface with new options

### Performance
- Up to 4x speedup on multi-core systems with parallel processing
- Reduced conflict misses by up to 25% with victim cache
- Improved write performance with combining buffer

### Technical Details
- Thread pool implementation using C++17 features
- Victim cache with LRU replacement and full statistics
- Write policies now support both update and allocation strategies
- Added M_PI definition for cross-platform compatibility in visualizations

## [1.1.0] - 2025-05-27

### Added
- **Comprehensive Replacement Policy Framework**: Implemented pluggable framework with LRU, FIFO, Random, and PLRU policies
- **Write Policy Support**: Added write-through policy alongside default write-back
- **Enhanced Error Handling**: Detailed error messages and recovery mechanisms
- **Performance Improvements**: Move semantics, perfect forwarding, and memory pool allocator
- **Configuration Validation**: Catch invalid parameters early with descriptive errors
- **Extended Statistics**: Cache efficiency metrics, write-through counters
- **Thread-Safe Logging**: Buffered output with string formatting support
- **Memory Pool Allocator**: Improved cache block management
- **JSON Trace Format**: Support for structured trace files
- **Cache Warmup**: Accurate benchmarking without cold-start effects
- **Interactive Mode**: Step-by-step simulation capability
- **Cache State Export**: Debugging functionality with detailed state dumps
- **GitHub Banners**: Professional SVG banners for repository social preview

### Changed
- Optimized cache lookup with improved hash function
- Enhanced prefetcher accuracy with better pattern detection
- Improved MESI protocol implementation with atomic operations
- Updated documentation with performance tuning guide
- Refactored trace parser for better performance on large files
- Modernized codebase with more C++17 features

### Fixed
- Memory leak in trace parser for malformed files
- Race condition in statistics collection
- Incorrect miss rate calculation for write-through caches
- Build warnings with newer compiler versions
- Edge case in LRU replacement policy

### Performance
- 25% improvement in simulation speed for large traces
- Reduced memory footprint by 15% through better data structures
- Optimized prefetcher reduces unnecessary memory traffic by 30%

## [1.0.0] - 2025-03-12

### Overview
*First stable release with full C++17 support*

This release marks the first stable version of the Cache Simulator, featuring a comprehensive simulation framework for cache and memory hierarchy systems. The simulator provides flexible configuration options, detailed statistics tracking, and advanced features such as adaptive prefetching and cache coherence.

### Added
- Initial release of the Cache Simulator with enhanced features
- Comprehensive CMake build system with C++17 support
- Modern project structure with separate source, test, and utility directories
- Configurable multi-level cache hierarchy
- Detailed statistics collection and reporting
- Parallel build support via CMake and Make
- Documentation system with markdown guides and examples

### Core Functionality
- `StreamBuffer` class for sequential prefetching implementation
- `StridePredictor` class for stride-based prefetching with confidence tracking
- `AdaptivePrefetcher` class providing a dynamic prefetching system that adjusts strategy based on workload
- `MESIProtocol` class implementing the MESI cache coherence protocol
- `Cache` class with support for various configurations and replacement policies
- `MemoryHierarchy` class orchestrating the entire cache hierarchy
- `TraceParser` utility for processing memory access traces
- LRU replacement policy implementation

### Developer Tools
- Unit testing framework for validating cache behavior
- Memory profiler for analyzing access patterns
- Trace parser utility for handling different memory trace formats
- Trace generator tool for creating test traces with various patterns
- Benchmarking utilities for performance comparison
- Visualization tools for statistics and cache behavior
- Git structure with proper .gitignore and organization
- Bash scripts for automation and batch simulation

### Documentation
- Detailed README with project overview, directory structure, and usage instructions
- Design documentation explaining architecture and algorithms
- Examples documentation with usage scenarios and case studies
- Code documentation with comprehensive comments
- CONTRIBUTING guide for new developers
- TODO list for future development

### Testing
- Unit tests for core components:
  - Cache hit/miss behavior
  - LRU replacement policy
  - Write-back functionality
  - Prefetching effectiveness
  - MESI protocol state transitions
- Validation tests with known trace patterns
- Performance benchmarks for different configurations

### Fixed
- Constructor initialization order in Cache class
- Memory hierarchy trace processing compatibility with modern interfaces
- Nodiscard attribute handling for method return values
- String_view temporary object lifetime issues

### Future Development
- Identified TODOs for future enhancements (see TODO.md for details):
  - Multi-processor simulation
  - Additional replacement policies
  - Victim cache implementation
  - Performance optimizations
  - Visualization tools and GUI
  - Power and area modeling

[1.0.0]: https://github.com/muditbhargava66/cache-simulator/releases/tag/v1.0.0