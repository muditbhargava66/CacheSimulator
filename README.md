<div align="center">

# Cache Simulator

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![C++17](https://img.shields.io/badge/C%2B%2B-17-orange)
![License](https://img.shields.io/badge/license-MIT-green)
![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square)
[![Contributors](https://img.shields.io/github/contributors/muditbhargava66/CacheSimulator)](https://github.com/muditbhargava66/CacheSimulator/graphs/contributors)
[![Last Commit](https://img.shields.io/github/last-commit/muditbhargava66/CacheSimulator)](https://github.com/muditbhargava66/CacheSimulator/commits/main)
[![Open Issues](https://img.shields.io/github/issues/muditbhargava66/CacheSimulator)](https://github.com/muditbhargava66/CacheSimulator/issues)
[![Open PRs](https://img.shields.io/github/issues-pr/muditbhargava66/CacheSimulator)](https://github.com/muditbhargava66/CacheSimulator/pulls)
[![GitHub stars](https://img.shields.io/github/stars/muditbhargava66/CacheSimulator)](https://github.com/muditbhargava66/CacheSimulator/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/muditbhargava66/CacheSimulator)](https://github.com/muditbhargava66/CacheSimulator/network/members)

**A comprehensive, high-performance cache and memory hierarchy simulator with advanced features for prefetching, cache coherence, and detailed performance analysis. Written in modern C++17.**
</div>

## Features

- **Configurable Cache Hierarchy**
  - Flexible L1 and L2 cache configurations
  - Adjustable block size, associativity, and cache size
  - Multiple replacement policies (LRU, Pseudo-LRU, FIFO)

- **Advanced Prefetching Mechanisms**
  - Stream buffer prefetching
  - Stride-based prefetching with pattern detection
  - Adaptive prefetching with multiple strategies
  - Dynamic prefetch distance adjustment

- **Cache Coherence Support**
  - MESI (Modified-Exclusive-Shared-Invalid) protocol implementation
  - Detailed tracking of coherence state transitions
  - Support for multi-processor simulations

- **Sophisticated Trace Analysis**
  - Memory access pattern detection
  - Detailed statistics gathering and reporting
  - Performance visualization tools
  - Trace file generation utilities

- **Modern C++17 Design**
  - Smart pointer memory management
  - Optional and variant for safer interfaces
  - String view for efficient text processing
  - Filesystem for portable file operations
  - Structured bindings and other modern features

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 19.14+)
- CMake 3.14+ (for CMake build) or GNU Make
- Bash shell for running the simulation scripts

## Building

### Using CMake (Recommended)

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest
```

### Using Make

```bash
# Build all targets
make

# Build with debug symbols
make debug

# Build and run tests
make test
make run_tests
```

## Usage

### Basic Usage

```bash
# Run with default configuration
./build/bin/cachesim <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <PREF_N> <PREF_M> <trace_file>

# Example
./build/bin/cachesim 64 32768 4 262144 8 1 4 traces/trace1.txt
```

Parameters:
- `BLOCKSIZE`: Cache block size in bytes (power of 2)
- `L1_SIZE`: L1 cache size in bytes
- `L1_ASSOC`: L1 cache associativity
- `L2_SIZE`: L2 cache size in bytes (0 to disable L2)
- `L2_ASSOC`: L2 cache associativity
- `PREF_N`: Enable prefetching (1) or disable (0)
- `PREF_M`: Prefetch distance
- `trace_file`: Path to memory access trace file

### Running Benchmark Scripts

```bash
# Run the benchmarking script
./scripts/run_simulations.sh

# Custom options
./scripts/run_simulations.sh --simulator ./build/bin/cachesim --traces ./my_traces --results ./my_results
```

### Generating Trace Files

```bash
# Generate a sequential trace
./build/bin/tools/trace_generator -o sequential.txt -p sequential -n 5000

# Generate a random trace
./build/bin/tools/trace_generator -o random.txt -p random -w 0.5

# Generate all standard trace patterns
./build/bin/tools/trace_generator --generate-all traces/generated/
```

## Trace File Format

The simulator reads memory access traces in the following format:

```
r|w <hex_address>
```

- `r`: Read operation
- `w`: Write operation
- `<hex_address>`: Memory address in hexadecimal (e.g., 0x1000)

Example:
```
r 0x1000
w 0x2000
r 0x1040
```

## Documentation

Detailed documentation is available in the `docs/` directory:

- [Design Documentation](docs/design.md): Architecture and design details
- [API Reference](docs/generated/html/index.html): Generated API documentation (requires running `make docs`)
- [Examples](docs/examples.md): Usage examples and case studies

## Project Structure

```
cache-simulator/
├── src/                    # Source code
│   ├── core/               # Core simulator components
│   ├── utils/              # Utility functions
│   └── main.cpp            # Main entry point
├── tests/                  # Tests
│   ├── unit/               # Unit tests
│   └── validation/         # Validation tests
├── tools/                  # Tools and utilities
│   └── trace_generator.cpp # Trace file generator
├── traces/                 # Example trace files
├── scripts/                # Scripts for automation
├── docs/                   # Documentation
├── CMakeLists.txt          # CMake build configuration
├── Makefile                # Make build configuration
└── README.md               # This file
```

## Performance Tips

- Use optimized builds for large trace files (`-O3` optimization)
- Adjust cache parameters based on the workload characteristics
- Enable prefetching for sequential or strided access patterns
- Use adaptive prefetching for mixed access patterns
- For large traces, consider using the batch processing mode

## Contributing

Contributions are welcome! Please feel free to submit a pull request.

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit your changes: `git commit -m 'Add amazing feature'`
4. Push to the branch: `git push origin feature/amazing-feature`
5. Open a pull request

Please ensure your code follows the project's coding style and includes appropriate tests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Inspired by academic cache simulation tools
- Built with modern C++17 features for performance and safety
- Special thanks to all contributors and testers

## Citation

If you use this simulator in your research, please cite it as:

```
@software{CacheSimulator,
  author = {Mudit Bhargava},
  title = {Cache Simulator: A C++17 Cache and Memory Hierarchy Simulator},
  year = {2025},
  url = {https://github.com/muditbhargava66/CacheSimulator}
}
```

## Contributing

Contributions are welcome! Please feel free to submit a pull request.

<div align="center">

---
⭐️ Star the repo and consider contributing!  
  
📫 **Contact**: [@muditbhargava66](https://github.com/muditbhargava66)
🐛 **Report Issues**: [Issue Tracker](https://github.com/muditbhargava66/CacheSimulator/issues)
  
© 2025 Mudit Bhargava. [MIT License](LICENSE)  
<!-- Copyright symbol using HTML entity for better compatibility -->
</div>