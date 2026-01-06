# Prefetching

The Cache Simulator supports multiple prefetching strategies to reduce cache miss latency.

## Overview

Prefetching brings data into the cache before it is explicitly requested, hiding memory latency for predictable access patterns.

## Prefetching Methods

### Stream Buffer Prefetching

Sequential access detection and prefetching.

**How it works:**
1. Detect sequential access patterns
2. Prefetch subsequent cache lines
3. Store in dedicated stream buffers

**Configuration:**

```json
{
  "l1": {
    "prefetching": {
      "enabled": true,
      "type": "stream",
      "distance": 4,
      "bufferSize": 4
    }
  }
}
```

**Parameters:**

| Parameter | Description | Default |
|-----------|-------------|---------|
| `distance` | Number of blocks to prefetch ahead | 4 |
| `bufferSize` | Stream buffer entries | 4 |

### Stride Prefetching

Pattern-based prefetching for regular stride accesses.

**How it works:**
1. Track address deltas between accesses
2. Detect repeating stride patterns
3. Predict and prefetch next addresses

**Configuration:**

```json
{
  "l1": {
    "prefetching": {
      "enabled": true,
      "type": "stride",
      "confidence": 2,
      "tableSize": 256
    }
  }
}
```

**Parameters:**

| Parameter | Description | Default |
|-----------|-------------|---------|
| `confidence` | Required pattern matches before prefetch | 2 |
| `tableSize` | Stride predictor table entries | 256 |

### Adaptive Prefetching

Dynamic strategy selection based on workload.

**How it works:**
1. Monitor prefetch accuracy
2. Adjust aggressiveness based on feedback
3. Switch strategies when patterns change

**Configuration:**

```json
{
  "l1": {
    "prefetching": {
      "enabled": true,
      "adaptive": true,
      "minAccuracy": 0.5,
      "aggressiveness": "medium"
    }
  }
}
```

**Aggressiveness levels:**
- `conservative`: Prefetch only on high confidence
- `medium`: Balance between coverage and accuracy
- `aggressive`: Prefetch on any pattern detection

## Command Line Usage

```bash
# Enable prefetching (default settings)
./cachesim 64 32768 4 262144 8 1 4 traces/sequential.txt
#                              P  D = Prefetch enabled, Distance 4

# Disable prefetching
./cachesim 64 32768 4 262144 8 0 0 traces/random.txt
```

## Prefetch Statistics

The simulator reports prefetching metrics:

```
Stream Buffer Statistics:
  Buffer Size: 4
  Hits: 1234
  Accesses: 5000
  Hit Rate: 24.7%
  
Prefetch Statistics:
  Prefetches Issued: 2500
  Useful Prefetches: 1850
  Accuracy: 74.0%
  Coverage: 46.3%
```

**Key metrics:**

| Metric | Description |
|--------|-------------|
| Hits | Accesses served from prefetched data |
| Accuracy | Useful prefetches / Total prefetches |
| Coverage | Prefetch hits / Total misses avoided |

## When to Use Prefetching

### Enable prefetching for:
- Sequential access patterns (array traversal)
- Regular stride patterns (matrix operations)
- Streaming workloads

### Disable prefetching for:
- Random access patterns
- Small working sets
- Memory-bandwidth constrained systems

## Performance Impact

Typical improvements with prefetching:

| Access Pattern | Miss Reduction | Notes |
|----------------|----------------|-------|
| Sequential | 50-80% | Best case for stream buffers |
| Strided | 30-60% | Depends on stride regularity |
| Random | 0-5% | Prefetching not effective |
| Mixed | 15-40% | Adaptive works best |

## Best Practices

1. **Analyze workload first** - Check access patterns before enabling
2. **Start conservative** - Begin with small prefetch distance
3. **Monitor accuracy** - Disable if accuracy drops below 50%
4. **Consider bandwidth** - Aggressive prefetching uses more bandwidth
5. **Use adaptive mode** - For workloads with varying patterns

## Troubleshooting

**Low prefetch accuracy:**
- Reduce prefetch distance
- Switch to stride-based for irregular patterns
- Enable adaptive mode

**No improvement from prefetching:**
- Verify workload has predictable patterns
- Check if working set fits in cache already
- Increase cache size before tuning prefetch

**Cache pollution from prefetching:**
- Reduce aggressiveness
- Use dedicated prefetch buffer
- Consider no-prefetch baseline

## See Also

- [Configuration](../user/configuration.md) - Full configuration options
- [Analysis Tools](../user/analysis.md) - Measuring prefetch effectiveness
- [User Guide](../user/user-guide.md) - Complete tutorial
