# Getting Started with Cache Simulator v1.2.2

## Installation

### Prerequisites
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.14 or higher
- Make, Ninja, or MSBuild

### Building from Source

#### Linux/macOS
```bash
git clone https://github.com/muditbhargava66/CacheSimulator.git
cd CacheSimulator
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
./bin/cachesim --version
```

#### Windows (PowerShell)
```powershell
git clone https://github.com/muditbhargava66/CacheSimulator.git
cd CacheSimulator
.\build.ps1
.\build\bin\cachesim.exe --version
```

### Quick Build Scripts

Platform-specific build scripts are provided:

- **Linux/Unix:** `./build.sh`
- **macOS:** `./build_macos.sh`
- **Windows:** `.\build.ps1` or `.\scripts\build_all.ps1`

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