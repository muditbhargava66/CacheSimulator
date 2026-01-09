# Getting Started

## Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.14 or higher
- Make, Ninja, or MSBuild

## Installation

### Linux/macOS

```bash
git clone https://github.com/muditbhargava66/CacheSimulator.git
cd CacheSimulator
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

### Windows (PowerShell)

```powershell
git clone https://github.com/muditbhargava66/CacheSimulator.git
cd CacheSimulator
.\build.ps1
```

### Verify Installation

```bash
./build/bin/cachesim --version
# Output: Cache Simulator v1.4.1
```

## Basic Usage

### Create a Trace File

Create `example.trace`:

```
# Memory access trace
R 0x1000
W 0x1004
R 0x1008
W 0x100C
R 0x1000  # Cache hit
```

### Run Simulation

```bash
# Default configuration
./build/bin/cachesim example.trace

# With visualization
./build/bin/cachesim --vis example.trace

# With power analysis
./build/bin/cachesim --power example.trace
```

### Command Line Parameters

```bash
./cachesim [OPTIONS] <trace_file>

Options:
  -h, --help              Show help message
  -v, --version           Show version
  -c, --config <file>     Use JSON configuration file
  --vis                   Enable cache visualization
  --power                 Enable power analysis
  --tech-node <nm>        Technology node (7, 14, 22, 32, 45)
```

## Quick Configuration

### Using Command Line

```bash
# Custom cache parameters
./cachesim 64 32768 4 262144 8 1 4 trace.txt
#         BS  L1   A1  L2   A2  P  D
# BS = Block Size, A1/A2 = Associativity, P = Prefetch, D = Distance
```

### Using JSON Config

Create `config.json`:

```json
{
  "l1": {
    "size": 32768,
    "associativity": 4,
    "blockSize": 64,
    "replacementPolicy": "LRU"
  },
  "l2": {
    "size": 262144,
    "associativity": 8,
    "blockSize": 64
  }
}
```

Run with config:

```bash
./cachesim --config config.json trace.txt
```

## Next Steps

- [User Guide](user-guide.md) - Complete tutorial
- [Configuration](configuration.md) - All configuration options
- [CLI Reference](cli-reference.md) - Full command reference
- [Examples](examples.md) - More usage examples
