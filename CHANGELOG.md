# Changelog

All notable changes to the Cache Simulator project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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