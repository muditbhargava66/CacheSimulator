# Changelog

All notable changes to the Cache Simulator project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.3] - 2026-02-10

### Fixed
- **Cache Associativity Bug** (Critical)
  - `findVictim()` now prefers empty (invalid) blocks before consulting the replacement policy
  - Previously the cache used only 1 way per set regardless of associativity, causing hit ratio to **decrease** with increasing associativity
  - `getTagAndSet()` now uses standard tag decomposition (`tag = blockNumber / numSets`) instead of storing the full block number as the tag, fixing incorrect writeback addresses

- **PLRU Replacement Policy**
  - Replaced floating-point `std::log2()` with integer bit-scan loop to prevent silent truncation (e.g. `log2(8)` returning `2.999` → `2`)
  - Added power-of-2 assertion in constructor since the tree structure requires it
  - Added missing `#include <cassert>`

- **Victim Cache `getAllValidAddresses()`**
  - Replaced `std::transform` + `std::remove(0)` with a simple validity-checked loop
  - Previously, a legitimate cache block at address 0 would be silently dropped

- **Victim Cache `invalidateBlocksInRange()`**
  - Changed from `std::remove_if` + `erase` (which physically removes elements) to in-place invalidation
  - The old approach corrupted `fifoQueue` and `addressToIndex` indices, causing incorrect evictions or crashes on subsequent operations

### Changed
- **Dead Code Removal**: Removed unused `lruOrder`, `fifoOrder`, and `nextFifoIndex` fields from `CacheSet` in `cache_block.h` and their initialization in `cache.cpp` — all replacement logic uses `ReplacementPolicyBase` instances
- **Namespace Cleanup**: Moved `VictimBlock` and `VictimCache` into `namespace cachesim`; removed dead `CacheSimulator` demo class from `victim_cache.h`
- **Code Simplification**: Removed stale "FIXED"/"SOLUTION" comments from `victim_cache.h` per code-simplifier guidelines
- Version updated to v1.4.3 in CLI, CMakeLists.txt, CITATION.cff, and documentation

### Added
- **Associativity Test**: New `tests/unit/core/associativity_test.cpp` verifying that hit ratio is monotonically non-decreasing as associativity increases
- **Cache Analysis Tool**: New `tools/cache_analysis.cpp` to visualize hit rate vs associativity and verify fix effectiveness

### Documentation
- Updated `README.md` "What's New" section for v1.4.3
- Updated `docs/developer/architecture.md` — corrected `CacheSet` description and refreshed Future Extensions
- Updated `docs/features/replacement-policies.md` — added PLRU power-of-2 requirement note
- Updated `docs/features/victim-cache.md` — code examples now reflect `cachesim` namespace

### Technical Details
- All coherence protocols (MSI, MESI, MOESI) were audited and verified correct
- All 20 tests pass (unit, integration, performance)

---

## [1.4.2] - 2026-02-02

### Fixed
- **MESI Statistics Not Shown** (GitHub Issue #16)
  - Added `recordStateTransition()` calls at all MESI state change points in `cache.cpp`
  - Write hits now properly record state transitions to Modified
  - Block installation now records transitions from Invalid to Modified/Exclusive
  - MESI state transition matrix is now correctly populated during simulation

### Added
- **Multiprocessor Trace Format Support**
  - `MemoryAccess` struct now includes optional `coreId` field
  - `TraceParser::parseLine()` extracts core ID from `PX r/w 0xADDR` format (e.g., `P0 r 0x1000`)
  - Enables multi-core trace simulation with proper processor identification

- **Trace Generator Multiprocessor Output**
  - New `--processors <count>` option for generating multiprocessor traces
  - Output uses `PX r/w 0xADDR` format compatible with multi-core simulator
  - Randomly distributes accesses across specified processors

- **MESI Statistics Unit Test**
  - Added `testMESIStatistics()` to `coherence_protocol_test.cpp`
  - Verifies `recordStateTransition()` correctly updates transition counts
  - Added `getTransitionCount()` getter to `MESIProtocol` class

### Changed
- Version updated to v1.4.2 in CLI, build scripts, and documentation
- **Code Quality Improvements**
  - Removed unused `inObject` variable in `config_utils.cpp`
  - Added `[[maybe_unused]]` attribute to reserved `logUnrecognizedKey()` function

## [1.4.1] - 2026-01-09

### Fixed
- **Config Parsing snake_case Support**
  - Added `normalizeConfigKey()` function to handle snake_case keys
  - Config files with `block_size`, `write_policy` now parse correctly
  - Maintains backward compatibility with camelCase keys

- **Config Version Metadata**
  - Updated all 8 example config files from `"version": "1.2.0"` to `"version": "1.4.0"`
  - Affected: full_features.json, high_performance.json, multiprocessor_4core.json,
    multiprocessor_system.json, nru_optimized.json, victim_cache_config.json,
    write_intensive.json, write_optimized.json

### Added
- **JSON Schema Validation**
  - `recognizedKeys` set for validating config key names
  - `logUnrecognizedKey()` function warns on unknown keys with section context
  - `findSimilarKey()` suggests correct key names for typos

- **Config Integration Tests**
  - New `tests/integration/config_parsing_test.cpp`
  - Tests all JSON config files parse successfully
  - Verifies snake_case normalization works correctly

- **User-Defined Profiler Regions**
  - `ProfilerRegionConfig` struct in `MemoryHierarchyConfig`
  - Fields: `startAddress`, `endAddress`, `name`
  - `profilerRegions` vector for custom address ranges

### Changed
- **License**: Changed from MIT to Apache License 2.0
- Enhanced error messages now suggest correct key format
- Added `<cstdint>` include to `memory_hierarchy.h` for `uint32_t`
- Removed version numbers from config file name/description fields

### Documentation
- **CITATION.cff**: Added for proper GitHub citation support (APA/BibTeX)
- **Side-Channel Research Guide**: New `docs/features/side-channel-research.md`
  - Flush+Reload attack simulation
  - Prime+Probe attack modeling
  - Spectre-like pattern analysis
  - Security research configurations

### Technical Details
- Config key normalization: `block_size` → `blockSize`
- All 19 tests pass (including new config integration test)
- All 5 example configs tested with real trace files

---

## [1.4.0] - 2026-01-08

### Added
- **L3 Cache Support**
  - Optional third cache level in `MemoryHierarchy`
  - Configurable via JSON with `l3` section
  - Inclusive L3 policy for multi-core coherence
  - Statistics: `getL3Misses()`, `getL3HitRate()`, `getL3MissRate()`

- **MSI/MOESI Coherence Protocols**
  - `CoherenceProtocolBase` abstract interface
  - `MSIProtocol`: 3-state protocol (Modified, Shared, Invalid)
  - `MOESIProtocol`: 5-state protocol (Modified, Owned, Exclusive, Shared, Invalid)
  - Factory pattern: `CoherenceProtocolBase::create(type)`
  - State transition tracking and statistics

- **Ring and Torus Interconnects**
  - `RingInterconnect`: Bidirectional ring with shortest path routing
  - `TorusInterconnect`: 2D torus with wrap-around connections
  - Proper latency modeling based on hop count
  - Full implementation of `InterconnectInterface`

- **CLI Parser Module**
  - Extracted from main.cpp to `utils/cli_parser.h/cpp`
  - `CLIParser` class with static methods
  - Improved code organization and testability

- **Cache Visualization Module**
  - Extracted from main.cpp to `utils/cache_visualization.h/cpp`
  - `CacheVisualization` class with cache state extraction and ASCII rendering
  - `CacheBlockState` struct for block metadata

- **New Unit Tests**
  - `tests/unit/core/coherence_protocol_test.cpp` - MSI/MOESI protocol tests
  - `tests/unit/core/l3_cache_test.cpp` - L3 cache tests
  - `tests/unit/multiprocessor/interconnect_test.cpp` - Ring/Torus tests
  - `tests/unit/utils/cli_parser_test.cpp` - CLI parsing tests

- **Multiprocessor Protocol Selection**
  - Added `coherenceProtocol` and `interconnectType` to `MultiProcessorSystem::Config`
  - Supports MSI, MESI, MOESI protocol selection
  - Supports Bus, Crossbar, Mesh, Ring, Torus interconnect selection

### Changed
- **Refactored main.cpp**: Reduced from 822 to 442 lines (-46%)
  - Now uses `CLIParser` for command-line parsing
  - Now uses `CacheVisualization` for cache state display
- Updated `memory_hierarchy.h` with L3 configuration support
- Updated `CMakeLists.txt` with new source files and tests
- Made Doxygen configuration conditional on file existence
- Updated documentation for all new features

### New Files
- `src/core/coherence_protocol.h/cpp` - Protocol base class and factory
- `src/core/msi_protocol.h/cpp` - MSI implementation
- `src/core/moesi_protocol.h/cpp` - MOESI implementation
- `src/utils/cli_parser.h/cpp` - CLI parsing module
- `src/utils/cache_visualization.h/cpp` - Cache visualization module
- `docs/features/l3-cache.md` - L3 cache documentation
- `docs/features/coherence-protocols.md` - Protocol documentation
- `docs/features/interconnects.md` - Interconnect documentation

### Technical Details
- MOESI Owned state enables dirty sharing without memory writeback
- Ring latency: `min(clockwise, counterclockwise) * hopLatency`
- Torus latency: `(wrapDx + wrapDy) * hopLatency`
- L3 inclusive policy maintains copy of all L1/L2 data
- All 18 tests pass (unit, integration, performance)

---

## [1.3.0] - 2026-01-07

### Added
- **Power and Area Modeling** (CACTI-inspired analytical models)
  - `PowerModel` class for energy calculation
    - Dynamic read/write energy per access (pJ)
    - Static leakage power with temperature scaling (mW)
    - Total energy accumulation during simulation (nJ)
    - Energy-Delay Product (EDP) metric
  - `AreaModel` class for silicon area estimation
    - Component breakdown: data array, tag array, decoders, sense amps, routing
    - Aspect ratio and layout geometry estimation
    - Cell efficiency metrics
  - Technology node support: 7nm, 14nm, 22nm, 32nm, 45nm
  - Constants derived from CACTI 7.0 and published research

- **CLI Integration**
  - `--power` flag to enable power and energy analysis
  - `--tech-node <nm>` flag to specify technology node (7, 14, 22, 32, 45)

- **Documentation**
  - New `docs/features/power-modeling.md` comprehensive feature guide
  - Updated `docs/user/configuration.md` with power config options

### Fixed
- **Visualization Rendering on Windows**
  - Replaced Unicode box-drawing characters (╔═║) with ASCII alternatives (+, -, |)
  - Added `TABLE_WIDTH` constant for consistent table alignment
  - Centered title row with proper padding
  - Footer stats now use `std::setw` for exact column alignment

### Changed
- Updated version to 1.3.0
- Updated C++ edition label to C++20

### New Files
- `src/models/power_constants.h` - Technology-specific parameters
- `src/models/power_model.h/cpp` - Power and energy modeling
- `src/models/area_model.h/cpp` - Area estimation
- `tests/unit/models/power_area_test.cpp` - Comprehensive unit tests (14 test cases)
- `docs/features/power-modeling.md` - Feature documentation

### Technical Details
- Bitline, wordline, decoder, and sense amplifier energy components
- Temperature-dependent leakage with exponential scaling
- 6T SRAM cell-based transistor count estimation
- Peripheral circuit overhead modeling

### Documentation Restructure
- Redesigned `docs/` folder structure with 16 organized files
- Created `docs/user/analysis.md` for performance analysis tools
- Created `docs/developer/building.md` for build instructions
- Created `docs/developer/api-reference.md` for code API
- Created `docs/features/prefetching.md` for prefetching documentation
- Removed version-specific `docs/features/v1.2.0-features.md` (content migrated)
- Updated `docs/developer/architecture.md` with C++20 and Power/Area models
- Removed emojis from all documentation headings
- Updated README.md with clean formatting and power model section

---

## [1.2.2] - 2026-01-06

### Added
- **Windows Platform Documentation**: Comprehensive guide for building and running on Windows
  - Build instructions for GCC, MSVC, and Clang
  - Known issues and solutions
  - Troubleshooting guide
- **Windows PowerShell Scripts**: Cross-platform build and benchmark scripts
  - `build.ps1`: Simple build script for Windows
  - `scripts/build_all.ps1`: Comprehensive build with options (Debug, Clean, Tests)
  - `scripts/run_benchmarks.ps1`: Benchmark runner for Windows

### Fixed
- **Cross-Platform Header Includes**: Added missing standard library headers for strict compiler compliance
  - `cache.h`: Added `#include <array>` for `std::array` usage
  - `logger.h`: Added `#include <array>` for `LogLevelNames`
  - `trace_utils.h`: Added `#include <functional>` for `std::function`
  - `interconnect.h`: Added `#include <cmath>`, `<cstdint>`, `<memory>`, `<optional>`
- **Windows Test Compatibility**: Fixed file locking issues in test files
  - `visualization_test.cpp`: Used RAII scope blocks to ensure `ifstream` is closed before `filesystem::remove()`
  - `cache_performance_test.cpp`: Used RAII scope blocks to ensure `TraceParser` releases file handle before cleanup
  - All 13 tests now pass on Windows
- **JSON Replacement Policy Parsing**: Fixed bug where JSON config files only recognized LRU and NRU policies
  - Added support for FIFO, Random, and PLRU replacement policies in the JSON parser
  - Both L1 and L2 cache configurations now correctly parse all replacement policy options
- **INI Replacement Policy Parsing**: Added missing replacement policy support to INI config parser
  - Both `replacement_policy` and `replacementPolicy` keys are now recognized
- **Profiler Region Naming**: Fixed confusing region names that implied L1/L2 cache mapping
  - Renamed to "Low/High Address Region" with explanatory comments
  - Added documentation that regions are for pattern analysis, not cache behavior
- **Trace Parser Inline Comments**: Fixed parser to handle inline comments (e.g., `r 0x1000 # comment`)
  - Previously, text after `#` on the same line was incorrectly parsed as a processor ID
  - Now properly strips inline comments before parsing
- **C++20 Standard**: Updated CMakeLists.txt to require C++20 for designated initializer support

### Technical Details
- Designated initializers (`.field = value`) in multiprocessor code require C++20
- The fixes ensure compatibility across Windows (GCC/MSVC), macOS (Clang/GCC), and Linux (GCC/Clang)
- No functional changes - only build and compatibility improvements

---

## [1.2.1] - 2025-09-02

### Fixed
- **Critical:** Fixed division by zero error in multiprocessor configurations ([#3](https://github.com/muditbhargava66/CacheSimulator/issues/3))
- **JSON Parser:** Enhanced configuration parser to handle nested JSON objects (`perCoreL1`, `sharedL2`)
- **Trace Parser:** Added support for multiprocessor trace format with processor ID prefix
- **Compatibility:** Maintains full backward compatibility with existing configuration and trace formats

### Technical Details
- Enhanced `src/utils/config_utils.cpp` to parse nested JSON configurations
- Updated `src/utils/trace_parser.cpp` to handle both 2-token and 3-token trace formats
- Added comprehensive validation to prevent cache configuration errors
- Supports multiple configuration formats: standard, multiprocessor, and legacy

### Testing
- [x] Original failing command now works: `./build/bin/cachesim --config configs/multiprocessor_4core.json traces/multiprocessor_coherence.txt`
- [x] Successfully processes 80 memory accesses from multiprocessor trace
- [x] Compatible with all existing configuration files
- [x] No performance regression

---

## [1.2.0] - 2025-07-19

### Added
- **NRU (Not Recently Used) Replacement Policy**: Complete implementation with reference bit tracking
- **No-Write-Allocate Policy**: Support for both write-through and write-back variants
- **Write Combining Buffer**: Coalescing writes to improve memory bandwidth utilization
- **Victim Cache**: Fully associative cache for storing evicted blocks, reducing conflict misses
- **Parallel Processing**: Thread pool and parallel trace processor for multi-core simulation
- **Statistical Charting**: ASCII line charts, pie charts, and scatter plots for data visualization
- **Enhanced Write Policy Framework**: Pluggable system supporting multiple allocation strategies
- **Parallel Benchmarking**: Compare multiple configurations simultaneously
- **Multi-processor Simulation**: Complete multi-core processor simulation with cache coherence
  - Processor core model with private L1 caches
  - Directory-based coherence controller implementing MESI protocol
  - Multiple interconnect topologies (Bus, Crossbar, Mesh)
  - Support for atomic operations and memory barriers
  - Comprehensive multi-processor statistics
- **Victim Cache Tests**: Complete unit test suite for victim cache functionality
- **Enhanced Documentation**:
  - Comprehensive victim cache documentation
  - Complete tutorial covering all features
  - Usage examples and best practices
- **Comprehensive Utility Scripts**:
  - Advanced build script with multiple configuration options
  - Comprehensive benchmark runner with parallel execution
  - Simulation runner with colorized output and CSV export
  - Trace validation script with error detection and fixing
  - Release creation script with cross-platform support
- **Enhanced Tool Suite**:
  - Advanced trace generator with multiple access patterns
  - Cache analyzer with optimization recommendations
  - Performance comparison tool with parallel benchmarking
  - Comprehensive documentation for all tools

### Changed
- Refactored write policy system to use inheritance-based design
- Reorganized project structure for better maintainability
- Enhanced visualization with new chart types
- Improved command-line interface with new options
- Fixed all compiler warnings and improved code quality
- Added support for cache state visualization in main simulation
- Standardized directory structure and naming conventions
- Removed all version-specific naming from files and directories
- Enhanced utility scripts with comprehensive features and error handling

### Performance
- Up to 4x speedup on multi-core systems with parallel processing
- Reduced conflict misses by up to 25% with victim cache
- Improved write performance with combining buffer
- Scalable multi-processor simulation supporting up to 64 cores

### Technical Details
- Thread pool implementation using C++17 features
- Victim cache with FIFO replacement and full statistics
- Write policies now support both update and allocation strategies
- Added M_PI definition for cross-platform compatibility in visualizations
- Complete MESI coherence protocol implementation
- Thread-safe directory-based coherence tracking
- Extensible interconnect framework with multiple topologies

### Fixed
- Fixed unused variable warnings in test files
- Fixed nodiscard attribute warnings
- Fixed type conversion issues between uint32_t and uint64_t
- Fixed lambda capture warnings in victim_cache.h
- Improved const-correctness throughout the codebase

---

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

---

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

---

## Future Development
- Identified TODOs for future enhancements (see TODO.md for details):
  - Multi-processor simulation
  - Additional replacement policies
  - Victim cache implementation
  - Performance optimizations
  - Visualization tools and GUI
  - Power and area modeling

[1.0.0]: https://github.com/muditbhargava66/cache-simulator/releases/tag/v1.0.0
