# Cache and Memory Hierarchy Simulator

This is a C++ implementation of a cache and memory hierarchy simulator. The simulator models the behavior of a two-level cache hierarchy with configurable parameters such as cache sizes, associativity, block size, and prefetching. It processes memory access traces and reports various statistics.

## Features

- Configurable L1 and L2 cache parameters (size, associativity, block size)
- Write-back and write-allocate policies
- LRU replacement policy
- Memory access traces processed from a file
- Computation of cache access statistics (hits, misses, reads, writes)
- Stream buffer prefetching support

## Requirements

- C++ compiler with C++11 support
- Make (optional, for using the provided Makefile)

## Usage

### Compilation

To compile the simulator, run the following command:

```bash
g++ -std=c++11 -O3 cachesim.cpp -o cachesim
```

Alternatively, you can use the provided Makefile by running:

```bash
make
```

### Running the Simulator

To run the simulator, use the following command:

```bash
./cachesim <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <PREF_N> <PREF_M> <trace_file>
```

- `BLOCKSIZE`: The size of each cache block in bytes (must be a power of 2)
- `L1_SIZE`: The total size of the L1 cache in bytes
- `L1_ASSOC`: The associativity of the L1 cache (1 for direct-mapped)
- `L2_SIZE`: The total size of the L2 cache in bytes (0 for no L2 cache)
- `L2_ASSOC`: The associativity of the L2 cache (1 for direct-mapped)
- `PREF_N`: Enable prefetching (1) or disable prefetching (0)
- `PREF_M`: Prefetch distance (number of blocks to prefetch ahead)
- `trace_file`: The path to the memory access trace file

Example:
```bash
./cachesim 64 1024 2 4096 4 1 8 traces/trace1.txt
```

### Trace File Format

The memory access trace file should contain one memory access per line, with the following format:

```
r|w <hex_address>
```

- `r` for a read access, `w` for a write access
- `hex_address`: The memory address in hexadecimal format

Example:
```
r 0x1234
w 0x5678
r 0xabcd
```

## Simulator Structure

The main components of the simulator are:

- `CacheBlock`: Represents a single cache block, storing the valid bit, dirty bit, tag, and data
- `CacheSet`: Represents a cache set, containing a vector of cache blocks and an LRU order
- `StreamBuffer`: Represents a stream buffer for prefetching, storing prefetched blocks and managing the buffer
- `Cache`: Represents a cache module with configurable size, associativity, block size, and prefetching support
- `MemoryHierarchy`: Represents the entire memory hierarchy, containing L1 and L2 cache instances

The `main` function parses the command-line arguments, creates a `MemoryHierarchy` instance, and processes the memory accesses from the trace file. Finally, it prints the cache access statistics.

## Prefetching Support

The simulator now includes support for stream buffer prefetching. When prefetching is enabled (`PREF_N` set to 1), the cache will attempt to prefetch blocks ahead of the current access based on the specified prefetch distance (`PREF_M`).

The `StreamBuffer` class manages the prefetched blocks and handles the prefetching logic. When a cache miss occurs and prefetching is enabled, the cache will check the stream buffer for the requested block. If found, it counts as a stream buffer hit. If not found, the cache will initiate a prefetch operation to bring the subsequent blocks into the stream buffer.

The cache access statistics now include stream buffer hits and prefetch-related information.

## Extending the Simulator

To add support for prefetching, you'll need to:

- [x] Implement a `StreamBuffer` class to represent stream buffers
- [x] Modify the `Cache` class to include prefetching logic and stream buffer integration
- [x] Update the `MemoryHierarchy` class to handle prefetching and collect relevant statistics
- [ ] Advanced prefetching algorithms like stride-based prefetching or adaptive prefetching that could potentially improve cache performance.
- [ ] Implementing a complete MESI (Modified-Exclusive-Shared-Invalid) protocol
- [ ] Validate the correctness of the cache simulator