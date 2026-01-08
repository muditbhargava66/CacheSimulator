# L3 Cache Support

## Overview

Version 1.4.0 introduces optional L3 (Last Level Cache) support to the Cache Simulator. The L3 cache serves as a shared cache in multi-core systems and provides an additional level of the memory hierarchy between L2 caches and main memory.

## Configuration

### JSON Configuration

```json
{
  "memory_hierarchy": {
    "l1": {
      "size": 32768,
      "associativity": 4,
      "block_size": 64
    },
    "l2": {
      "size": 262144,
      "associativity": 8,
      "block_size": 64
    },
    "l3": {
      "size": 8388608,
      "associativity": 16,
      "block_size": 64
    },
    "l3_inclusive": true
  }
}
```

### Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `size` | int | L3 cache size in bytes (typical: 4MB-32MB) |
| `associativity` | int | Set associativity (typical: 8-16 ways) |
| `block_size` | int | Cache line size (must match L1/L2) |
| `l3_inclusive` | bool | Whether L3 is inclusive of L1/L2 |

## Inclusion Policy

### Inclusive L3

When `l3_inclusive` is `true`, the L3 cache maintains a superset of all data in L1 and L2 caches:

- **Benefit**: Simplifies coherence in multi-core systems
- **Trade-off**: Evictions from L3 require back-invalidation of L1/L2

### Non-Inclusive L3

When `l3_inclusive` is `false`:

- L3 acts as a victim cache for L2 misses
- No back-invalidation required on L3 eviction
- More efficient use of total cache capacity

## Access Flow

```
L1 Access
    ├── HIT → Return data
    └── MISS → L2 Access
                ├── HIT → Return data, update L1
                └── MISS → L3 Access
                            ├── HIT → Return data, update L1, L2
                            └── MISS → Memory Access
```

## Statistics

The following L3 statistics are available:

| Method | Description |
|--------|-------------|
| `getL3Misses()` | Total L3 cache misses |
| `getL3HitRate()` | L3 hit rate (0.0 to 1.0) |
| `getL3MissRate()` | L3 miss rate (0.0 to 1.0) |
| `hasL3()` | Whether L3 is configured |
| `getL3Cache()` | Optional pointer to L3 cache |

## Typical Configurations

### Desktop/Laptop (Multi-Core)

```json
{
  "l3": {
    "size": 8388608,
    "associativity": 16,
    "block_size": 64
  },
  "l3_inclusive": true
}
```

### Server (Many-Core)

```json
{
  "l3": {
    "size": 33554432,
    "associativity": 16,
    "block_size": 64
  },
  "l3_inclusive": false
}
```

## Implementation Details

### Memory Hierarchy Integration

The L3 cache is implemented as an `std::optional<Cache>` member in `MemoryHierarchy`:

```cpp
std::optional<Cache> l3;
bool l3Inclusive_ = true;
```

### Access Logic

```cpp
if (!l1Hit) {
    if (l2 && !l2->access(address, isWrite)) {
        if (l3) {
            l3Accesses++;
            if (!l3->access(address, isWrite)) {
                l3Misses++;
                // Memory access
            }
        }
    }
}
```

## Performance Considerations

1. **L3 Size**: Should be significantly larger than total L2 capacity
2. **Associativity**: Higher associativity reduces conflict misses
3. **Inclusivity**: Inclusive is simpler but wastes capacity duplicating L1/L2 data

## See Also

- [Memory Hierarchy Configuration](../user/configuration.md)
- [Multiprocessor Simulation](multiprocessor.md)
- [Cache Coherence Protocols](coherence-protocols.md)
