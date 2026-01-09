# Side-Channel Attack Research with Cache Simulator

This document describes how to use the Cache Simulator for side-channel attack research, including modeling timing-based attacks like Flush+Reload, Prime+Probe, and analyzing vulnerabilities like Spectre and Meltdown.

## Overview

Cache timing side-channel attacks exploit variations in memory access times to infer sensitive information. The Cache Simulator provides the infrastructure to:

- Model L1/L2/L3 cache behavior with precise timing
- Simulate cache set conflicts and evictions
- Analyze access patterns for information leakage
- Evaluate cache partitioning countermeasures
- Study coherence protocol vulnerabilities

## Key Attack Techniques

### 1. Flush+Reload Attack

**Concept**: The attacker flushes a cache line, waits, then reloads and measures access time to determine if the victim accessed the same memory.

**Simulation Approach**:
```cpp
// Create a shared memory region
MemoryHierarchy cache(config);

// Step 1: Prime - Access attacker's data
cache.access(0x1000, false);  // Read to load into cache

// Step 2: Flush - Evict the cache line
// (Simulate by filling cache set with other data)
for (int i = 1; i <= associativity; i++) {
    cache.access(0x1000 + i * stride, false);
}

// Step 3: Victim activity (external process)
// ...

// Step 4: Reload - Measure access time
auto start = high_resolution_clock::now();
cache.access(0x1000, false);
auto end = high_resolution_clock::now();

// Fast access = victim used this address (cache hit)
// Slow access = victim didn't use it (cache miss)
```

### 2. Prime+Probe Attack

**Concept**: The attacker fills a cache set with their data, waits, then probes to see if victim activity evicted any lines.

**Simulation Approach**:
```cpp
MemoryHierarchy cache(config);

// Calculate addresses that map to the same cache set
uint32_t targetSet = 5;  // Target cache set index
uint32_t setSize = cacheSize / (numSets * blockSize);

// Step 1: Prime - Fill entire cache set
std::vector<uint32_t> primeAddresses;
for (int way = 0; way < associativity; way++) {
    uint32_t addr = (targetSet * blockSize) + (way * numSets * blockSize);
    primeAddresses.push_back(addr);
    cache.access(addr, false);
}

// Step 2: Wait for victim activity
// ...

// Step 3: Probe - Re-access prime addresses and measure timing
int evictionCount = 0;
for (uint32_t addr : primeAddresses) {
    if (measureAccessTime(cache, addr) > CACHE_HIT_THRESHOLD) {
        evictionCount++;  // Victim evicted this line
    }
}
```

### 3. Analyzing Spectre-like Patterns

**Concept**: Speculative execution can leave cache side effects even when speculatively accessed data is architecturally discarded.

**Simulation Approach**:
```cpp
// Simulate speculative access pattern
uint8_t secret = getSecretByte();
uint32_t probeArray[256 * CACHE_LINE_SIZE];

// Speculatively access probe array indexed by secret
// Even if bounds check fails, cache state changes
cache.access((uint32_t)&probeArray[secret * CACHE_LINE_SIZE], false);

// Attacker probes all 256 possible values
for (int i = 0; i < 256; i++) {
    auto time = measureAccessTime(cache, (uint32_t)&probeArray[i * CACHE_LINE_SIZE]);
    if (time < CACHE_HIT_THRESHOLD) {
        printf("Secret byte likely: %d\n", i);
    }
}
```

## Configuration for Security Research

### Recommended Config for Attack Analysis
```json
{
  "name": "Side-Channel Research Config",
  "version": "1.4.0",

  "l1": {
    "size": 32768,
    "associativity": 8,
    "blockSize": 64,
    "replacementPolicy": "LRU"
  },

  "l2": {
    "size": 262144,
    "associativity": 8,
    "blockSize": 64
  },

  "l3": {
    "size": 8388608,
    "associativity": 16,
    "blockSize": 64
  },

  "profilerRegions": [
    {
      "startAddress": "0x00000",
      "endAddress": "0x10000",
      "name": "AttackerRegion"
    },
    {
      "startAddress": "0x10000",
      "endAddress": "0x20000",
      "name": "VictimRegion"
    },
    {
      "startAddress": "0x20000",
      "endAddress": "0x30000",
      "name": "SharedRegion"
    }
  ]
}
```

## Countermeasure Evaluation

### 1. Cache Partitioning
```json
{
  "l3": {
    "partitioning": "way-based",
    "securityDomains": 2
  }
}
```

### 2. Constant-Time Access Patterns
Analyze whether your cryptographic implementation has data-dependent cache access patterns.

### 3. Cache Line Locking
Evaluate defense mechanisms that lock critical cache lines.

## Trace File Format for Security Analysis

Create traces that simulate attacker-victim interactions:

```
# Attacker: Prime phase
r 0x1000
r 0x2000
r 0x3000
r 0x4000

# Victim: Secret-dependent access
r 0x5000  # Accesses based on secret key bit

# Attacker: Probe phase
r 0x1000
r 0x2000
r 0x3000
r 0x4000
```

## Research Applications

| Application        | Simulator Features Used                        |
| ------------------ | ---------------------------------------------- |
| Attack modeling    | Cache hit/miss tracking, set index calculation |
| Defense evaluation | Replacement policies, partitioning             |
| Timing analysis    | Access latency modeling                        |
| Multicore attacks  | MESI/MOESI coherence protocols                 |
| LLC attacks        | L3 cache with inclusive policy                 |

## References

- Yarom & Falkner, "FLUSH+RELOAD: A High Resolution, Low Noise, L3 Cache Side-Channel Attack" (2014)
- Liu et al., "Last-Level Cache Side-Channel Attacks are Practical" (IEEE S&P 2015)
- Kocher et al., "Spectre Attacks: Exploiting Speculative Execution" (2018)
- Lipp et al., "Meltdown: Reading Kernel Memory from User Space" (2018)

## Getting Help

For security research questions:
- Open an issue with the `security-research` label
- See [examples/security](../examples/security) for sample attack traces
