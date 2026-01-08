# Coherence Protocols

## Overview

Version 1.4.0 introduces a flexible coherence protocol framework supporting MSI, MESI, and MOESI protocols. These protocols ensure cache coherence in multi-processor systems.

## Protocol Comparison

| Protocol | States | Description |
|----------|--------|-------------|
| MSI | 3 | Basic protocol: Modified, Shared, Invalid |
| MESI | 4 | Adds Exclusive state for silent upgrades |
| MOESI | 5 | Adds Owned state for dirty sharing |

## State Definitions

### MSI States

| State | Description |
|-------|-------------|
| **Modified** | Line is dirty, only copy in system |
| **Shared** | Line is clean, may exist in other caches |
| **Invalid** | Line is not valid |

### MOESI States

| State | Description |
|-------|-------------|
| **Modified** | Line is dirty, only copy in system |
| **Owned** | Line is dirty, shared with other caches |
| **Exclusive** | Line is clean, only copy in system |
| **Shared** | Line is clean, may exist in other caches |
| **Invalid** | Line is not valid |

## State Transition Diagrams

### MSI Transitions

```
                    +------------------+
                    |                  |
        Local Read  |    Shared (S)    |  Remote Write
    +-------------->|                  |<---------------+
    |               +--------+---------+                |
    |                        |                          |
    |                        | Local Write              |
    |                        v                          |
+---+------+         +-------+--------+         +-------+------+
|          |         |                |         |              |
| Invalid  |<--------+  Modified (M)  +-------->|   Invalid    |
|   (I)    |  Remote |                | Remote  |     (I)      |
|          |  Write  +----------------+  Read   |              |
+----------+                                    +--------------+
```

### MOESI Key Transition

The unique MOESI feature: Modified → Owned on remote read:

```
Modified --[Remote Read]--> Owned (instead of Shared)
```

This allows dirty sharing without memory writeback.

## Usage

### Creating a Protocol

```cpp
#include "core/coherence_protocol.h"

// Using the factory
auto protocol = CoherenceProtocolBase::create(CoherenceProtocolType::MOESI);

// Direct instantiation
MOESIProtocol moesi;
MSIProtocol msi;
```

### Handling Transitions

```cpp
int state = static_cast<int>(MOESIState::Invalid);

// Local read (no shared copy)
state = protocol->handleLocalRead(state, false);
// state is now Exclusive

// Local write
state = protocol->handleLocalWrite(state);
// state is now Modified

// Remote read (another processor reads)
state = protocol->handleRemoteRead(state);
// state is now Owned (MOESI) or Shared (MSI/MESI)
```

## Configuration

### JSON Configuration

```json
{
  "multiprocessor": {
    "coherence_protocol": "MOESI",
    "num_processors": 4
  }
}
```

### Protocol Selection

| Value | Protocol |
|-------|----------|
| `"MSI"` | 3-state MSI |
| `"MESI"` | 4-state MESI |
| `"MOESI"` | 5-state MOESI |

## Performance Characteristics

### MSI

- **Pros**: Simplest, lowest overhead
- **Cons**: No silent upgrades, high bus traffic

### MESI

- **Pros**: Silent Shared→Modified upgrade when exclusive
- **Cons**: Extra state tracking

### MOESI

- **Pros**: Dirty sharing reduces memory traffic
- **Cons**: Most complex, highest state overhead

## Statistics

Each protocol tracks:

```cpp
protocol->printStats();
// Output:
// === MOESI Protocol Statistics ===
// Total Transitions: 1000
// State Distribution:
//   Modified:  150 (15.0%)
//   Owned:      50 (5.0%)
//   Exclusive: 200 (20.0%)
//   Shared:    400 (40.0%)
//   Invalid:   200 (20.0%)
```

## Implementation

### Class Hierarchy

```
CoherenceProtocolBase (abstract)
├── MSIProtocol
├── MOESIProtocol
└── MESIProtocolAdapter (wraps legacy MESI)
```

### Factory Pattern

```cpp
std::unique_ptr<CoherenceProtocolBase> 
CoherenceProtocolBase::create(CoherenceProtocolType type) {
    switch (type) {
        case CoherenceProtocolType::MSI:
            return std::make_unique<MSIProtocol>();
        case CoherenceProtocolType::MESI:
            return std::make_unique<MESIProtocolAdapter>();
        case CoherenceProtocolType::MOESI:
            return std::make_unique<MOESIProtocol>();
    }
}
```

## See Also

- [Multiprocessor Simulation](multiprocessor.md)
- [Interconnect Topologies](interconnects.md)
- [L3 Cache](l3-cache.md)
