# API Reference

This document provides an overview of the main classes and APIs in the Cache Simulator.

## Core Classes

### Cache

The main cache simulation class.

```cpp
namespace cachesim {

class Cache {
public:
    // Constructor
    explicit Cache(const CacheConfig& config);
    
    // Main access method
    AccessResult access(uint64_t address, bool isWrite);
    
    // Statistics
    CacheStats getStats() const;
    void printStats() const;
    void resetStats();
    
    // Configuration
    uint32_t getNumSets() const;
    uint32_t getAssociativity() const;
    uint32_t getBlockSize() const;
    
    // State inspection
    std::vector<CacheBlockState> getBlockStates() const;
    
    // Power modeling (v1.3.0)
    void enablePowerModeling(int techNode);
    bool isPowerModelEnabled() const;
    PowerStats getPowerStats() const;
};

}
```

### CacheConfig

Configuration structure for cache initialization.

```cpp
struct CacheConfig {
    uint32_t size;              // Cache size in bytes
    uint32_t associativity;     // Number of ways
    uint32_t blockSize;         // Block size in bytes
    ReplacementPolicy replacementPolicy;  // LRU, FIFO, NRU, etc.
    WritePolicy writePolicy;    // WriteBack or WriteThrough
    bool prefetchingEnabled;    // Enable prefetching
    uint32_t prefetchDistance;  // Prefetch distance
};
```

### MemoryHierarchy

Manages multi-level cache hierarchy.

```cpp
class MemoryHierarchy {
public:
    MemoryHierarchy(const MemoryHierarchyConfig& config);
    
    // Access through hierarchy
    AccessResult access(uint64_t address, bool isWrite);
    
    // Statistics
    void printStats() const;
    HierarchyStats getStats() const;
    
    // Cache access
    Cache* getL1Cache();
    Cache* getL2Cache();
    Cache* getL3Cache();
};
```

## Enumerations

### ReplacementPolicy

```cpp
enum class ReplacementPolicy {
    LRU,      // Least Recently Used
    FIFO,     // First In First Out
    Random,   // Random replacement
    PLRU,     // Pseudo-LRU (tree-based)
    NRU       // Not Recently Used
};
```

### WritePolicy

```cpp
enum class WritePolicy {
    WriteBack,     // Write to cache, mark dirty
    WriteThrough   // Write to cache and memory
};
```

### CacheMissType

```cpp
enum class CacheMissType {
    Compulsory,   // First access to block
    Capacity,     // Cache too small
    Conflict,     // Set conflict
    Coherence     // Invalidated by another processor
};
```

## Power Modeling API (v1.3.0)

### PowerModel

```cpp
class PowerModel {
public:
    PowerModel(const CacheConfig& config, int techNode);
    
    // Record access
    void recordAccess(bool isRead, bool isHit);
    
    // Update simulation time
    void updateSimulationTime(double seconds);
    
    // Get statistics
    PowerStats getStats() const;
    std::string getReport() const;
    
    // Energy metrics
    double getDynamicEnergy() const;      // Total dynamic energy (nJ)
    double getLeakagePower() const;       // Static power (mW)
    double getEnergyDelayProduct() const; // EDP metric
};
```

### AreaModel

```cpp
class AreaModel {
public:
    AreaModel(const CacheConfig& config, int techNode);
    
    // Get area breakdown
    AreaBreakdown getAreaBreakdown() const;
    double getTotalArea() const;          // Total area (mm^2)
    
    // Component areas
    double getDataArrayArea() const;
    double getTagArrayArea() const;
    double getDecoderArea() const;
    double getSenseAmpArea() const;
};
```

## Trace Processing

### TraceParser

```cpp
class TraceParser {
public:
    TraceParser(const std::string& filename);
    
    // Parse trace file
    bool parse();
    
    // Get parsed entries
    const std::vector<TraceEntry>& getEntries() const;
    
    // Iteration
    bool hasNext() const;
    TraceEntry next();
};
```

### TraceEntry

```cpp
struct TraceEntry {
    uint64_t address;       // Memory address
    bool isWrite;           // Write operation
    uint32_t processorId;   // Processor ID (multiprocessor)
};
```

## Multiprocessor API

### MultiprocessorSimulator

```cpp
class MultiprocessorSimulator {
public:
    MultiprocessorSimulator(const MultiprocessorConfig& config);
    
    // Access from specific processor
    AccessResult access(uint32_t processorId, 
                       uint64_t address, 
                       bool isWrite);
    
    // Coherence statistics
    CoherenceStats getCoherenceStats() const;
    void printCoherenceStats() const;
};
```

## Usage Examples

### Basic Cache Simulation

```cpp
#include "core/cache.h"

using namespace cachesim;

int main() {
    CacheConfig config;
    config.size = 32768;           // 32KB
    config.associativity = 4;      // 4-way
    config.blockSize = 64;         // 64B blocks
    config.replacementPolicy = ReplacementPolicy::LRU;
    
    Cache cache(config);
    
    // Simulate accesses
    cache.access(0x1000, false);  // Read
    cache.access(0x1004, true);   // Write
    
    // Print results
    cache.printStats();
    return 0;
}
```

### Power Analysis

```cpp
#include "core/cache.h"
#include "models/power_model.h"

Cache cache(config);
cache.enablePowerModeling(45);  // 45nm technology

// Run simulation
for (const auto& entry : trace) {
    cache.access(entry.address, entry.isWrite);
}

// Get power statistics
auto stats = cache.getPowerStats();
std::cout << "Dynamic Energy: " << stats.totalEnergy << " nJ\n";
std::cout << "Leakage Power: " << stats.leakagePower << " mW\n";
```

## See Also

- [Architecture](architecture.md) - System design overview
- [Building](building.md) - Build instructions
- [User Guide](../user/user-guide.md) - Usage tutorial
