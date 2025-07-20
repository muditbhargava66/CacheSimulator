# Getting Started with Cache Simulator v1.2.0

## Installation

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.14 or higher
- Make or Ninja build system

### Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/muditbhargava66/CacheSimulator.git
   cd CacheSimulator
   ```

2. **Build the project:**
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j$(nproc)
   ```

3. **Verify installation:**
   ```bash
   ./bin/cachesim --version
   ```

### Quick Build Scripts

For convenience, platform-specific build scripts are provided:

- **macOS:** `./build_macos.sh`
- **Linux/Unix:** `./build.sh`

## Basic Usage

### Simple Simulation

Create a trace file `example.trace`:
```
# Simple memory trace
R 0x1000
W 0x1004
R 0x1008
W 0x100C
```

Run the simulation:
```bash
./bin/cachesim example.trace
```

### Command Line Options

```bash
# Show help
./bin/cachesim --help

# Run with visualization
./bin/cachesim --visualize example.trace

# Enable victim cache
./bin/cachesim --victim-cache example.trace

# Run benchmark comparison
./bin/cachesim --benchmark example.trace

# Export results to CSV
./bin/cachesim --export results.csv example.trace
```

### Configuration File

Create `config.json`:
```json
{
  "l1": {
    "size": 32768,
    "associativity": 4,
    "blockSize": 64,
    "replacementPolicy": "NRU"
  },
  "l2": {
    "size": 262144,
    "associativity": 8,
    "blockSize": 64
  },
  "victimCache": {
    "enabled": true,
    "size": 8
  }
}
```

Run with configuration:
```bash
./bin/cachesim --config config.json example.trace
```

## Next Steps

- Read the [User Guide](user-guide.md) for detailed usage instructions
- Explore [Configuration Options](configuration.md) for advanced settings
- Check out [Examples](examples.md) for common use cases
- Review [v1.2.0 Features](../features/v1.2.0-features.md) for new capabilities