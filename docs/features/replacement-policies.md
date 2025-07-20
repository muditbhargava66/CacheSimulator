# Replacement Policies

The Cache Simulator supports multiple cache replacement policies through a pluggable framework.

## Available Policies

### LRU (Least Recently Used)
- **Description**: Evicts the least recently used block
- **Implementation**: Maintains access order for each set
- **Best for**: General-purpose workloads with temporal locality
- **Configuration**: `"replacementPolicy": "LRU"`

### FIFO (First In, First Out)
- **Description**: Evicts the oldest block in the set
- **Implementation**: Tracks installation order
- **Best for**: Streaming workloads with no reuse
- **Configuration**: `"replacementPolicy": "FIFO"`

### NRU (Not Recently Used) - New in v1.2.0
- **Description**: Uses reference bits to track recent usage
- **Implementation**: Periodically clears reference bits
- **Best for**: Workloads with mixed access patterns
- **Configuration**: `"replacementPolicy": "NRU"`

### Random
- **Description**: Randomly selects a block for eviction
- **Implementation**: Uses random number generator
- **Best for**: Simple implementation, unpredictable workloads
- **Configuration**: `"replacementPolicy": "Random"`

### PLRU (Pseudo-LRU)
- **Description**: Approximates LRU using binary tree
- **Implementation**: Tree-based tracking with fewer bits
- **Best for**: Hardware-efficient LRU approximation
- **Configuration**: `"replacementPolicy": "PLRU"`

## Performance Comparison

| Policy | Memory Overhead | Complexity | Typical Hit Rate |
|--------|----------------|------------|------------------|
| LRU    | High           | O(log n)   | Highest          |
| FIFO   | Low            | O(1)       | Medium           |
| NRU    | Low            | O(1)       | High             |
| Random | Minimal        | O(1)       | Low              |
| PLRU   | Medium         | O(log n)   | High             |

## Configuration Examples

### Basic Configuration
```json
{
  "l1": {
    "replacementPolicy": "NRU"
  },
  "l2": {
    "replacementPolicy": "LRU"
  }
}
```

### Mixed Policy Configuration
```json
{
  "l1": {
    "size": 32768,
    "associativity": 4,
    "replacementPolicy": "NRU"
  },
  "l2": {
    "size": 262144,
    "associativity": 8,
    "replacementPolicy": "LRU"
  }
}
```

## Implementation Details

### Policy Interface
```cpp
class ReplacementPolicyBase {
public:
    virtual void onAccess(int blockIndex) = 0;
    virtual void onInstall(int blockIndex) = 0;
    virtual int selectVictim(const std::vector<bool>& validBlocks) = 0;
    virtual void reset() = 0;
};
```

### Adding Custom Policies
1. Inherit from `ReplacementPolicyBase`
2. Implement required methods
3. Register in `ReplacementPolicyFactory`
4. Add configuration support

## Benchmarking Results

Performance comparison on typical workloads:

### SPEC CPU2017 Traces
- **LRU**: 85.2% hit rate
- **NRU**: 83.7% hit rate
- **FIFO**: 78.1% hit rate
- **Random**: 72.3% hit rate
- **PLRU**: 84.1% hit rate

### Matrix Multiplication
- **LRU**: 92.1% hit rate
- **NRU**: 91.8% hit rate
- **FIFO**: 89.2% hit rate
- **Random**: 76.4% hit rate
- **PLRU**: 91.5% hit rate

## Recommendations

### General Purpose
- **Primary**: LRU for highest performance
- **Alternative**: NRU for good performance with lower overhead

### Embedded Systems
- **Primary**: FIFO for simplicity
- **Alternative**: NRU for better performance

### High-Associativity Caches
- **Primary**: PLRU for hardware efficiency
- **Alternative**: NRU for software implementation

### Research and Experimentation
- **Primary**: Random for baseline comparison
- **Alternative**: Custom policies for specific workloads