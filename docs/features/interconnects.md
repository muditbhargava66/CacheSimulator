# Interconnect Topologies

## Overview

Version 1.4.0 adds Ring and Torus interconnect topologies to the simulator, complementing the existing Bus, Crossbar, and Mesh options.

## Topology Comparison

| Topology | Scalability | Latency | Bandwidth | Complexity |
|----------|-------------|---------|-----------|------------|
| Bus | Low | O(1) | Limited | Simple |
| Crossbar | Medium | O(1) | High | High cost |
| Mesh | High | O(√N) | Good | Moderate |
| Ring | Medium | O(N/2) | Moderate | Simple |
| Torus | High | O(√N/2) | Good | Moderate |

## Ring Interconnect

### Description

A bidirectional ring connects all nodes in a circular topology. Messages can travel clockwise or counter-clockwise, taking the shorter path.

### Latency Model

```
latency = min(clockwise_hops, counterclockwise_hops) × hop_latency

where:
  clockwise_hops = (dest - source + N) % N
  counterclockwise_hops = N - clockwise_hops
```

### Example (8 nodes)

```
     0
   /   \
  7     1
  |     |
  6     2
  |     |
  5     3
   \   /
     4

0 → 2: min(2, 6) = 2 hops
0 → 6: min(6, 2) = 2 hops
0 → 4: min(4, 4) = 4 hops
```

## Torus Interconnect

### Description

A 2D torus is a mesh with wrap-around connections. Each node connects to 4 neighbors, with edge nodes wrapping to the opposite side.

### Latency Model

```
latency = (wrap_dx + wrap_dy) × hop_latency

where:
  wrap_dx = min(|dx|, width - |dx|)
  wrap_dy = min(|dy|, height - |dy|)
```

### Example (4×4 Torus)

```
 0 -- 1 -- 2 -- 3 --+
 |    |    |    |   |
 4 -- 5 -- 6 -- 7   |
 |    |    |    |   |
 8 -- 9 --10 --11   |
 |    |    |    |   |
12 --13 --14 --15   |
 |                  |
 +------------------+

0 → 3: dx=3, wrap=min(3,1)=1 hop
0 → 15: (3,3) → wrap=(1,1)=2 hops
```

## Usage

### Creating Interconnects

```cpp
#include "core/multiprocessor/interconnect.h"

// Using the factory
auto ring = InterconnectFactory::create(
    InterconnectType::Ring,
    8,   // num nodes
    2    // hop latency
);

auto torus = InterconnectFactory::create(
    InterconnectType::Torus,
    16,  // num nodes (must be perfect square)
    2    // hop latency
);
```

### Sending Messages

```cpp
InterconnectMessage msg;
msg.sourceId = 0;
msg.destId = 5;
msg.type = InterconnectMessage::Type::DataTransfer;
msg.data = cacheLineData;

uint32_t latency = ring->sendMessage(msg);
```

## Configuration

### JSON Configuration

```json
{
  "multiprocessor": {
    "num_processors": 16,
    "interconnect": {
      "type": "torus",
      "hop_latency": 2
    }
  }
}
```

### Interconnect Types

| Value | Type |
|-------|------|
| `"bus"` | Shared bus |
| `"crossbar"` | Full crossbar |
| `"mesh"` | 2D mesh |
| `"ring"` | Bidirectional ring |
| `"torus"` | 2D torus |

## Performance Characteristics

### Ring

- **Best for**: 4-16 nodes
- **Pros**: Simple routing, uniform bisection
- **Cons**: Latency grows with node count

### Torus

- **Best for**: 16-64 nodes
- **Pros**: Lower diameter than mesh, good scalability
- **Cons**: Complex wrap-around wiring

## Statistics

```cpp
auto stats = interconnect->getStats();
std::cout << "Total Messages: " << stats.totalMessages << std::endl;
std::cout << "Total Latency: " << stats.totalLatency << std::endl;
std::cout << "Avg Hop Count: " << stats.avgHopCount << std::endl;
std::cout << "Utilization: " << stats.utilization << std::endl;
```

## Implementation Details

### RingInterconnect

```cpp
class RingInterconnect : public InterconnectInterface {
    uint32_t calculateLatency(uint32_t src, uint32_t dst) {
        int clockwise = (dst - src + numNodes) % numNodes;
        int counterClockwise = numNodes - clockwise;
        return std::min(clockwise, counterClockwise) * hopLatency;
    }
};
```

### TorusInterconnect

```cpp
class TorusInterconnect : public InterconnectInterface {
    uint32_t calculateLatency(uint32_t src, uint32_t dst) {
        int sx = src % width, sy = src / width;
        int dx = dst % width, dy = dst / width;
        int distX = std::min(std::abs(dx-sx), width - std::abs(dx-sx));
        int distY = std::min(std::abs(dy-sy), height - std::abs(dy-sy));
        return (distX + distY) * hopLatency;
    }
};
```

## See Also

- [Multiprocessor Simulation](multiprocessor.md)
- [Coherence Protocols](coherence-protocols.md)
