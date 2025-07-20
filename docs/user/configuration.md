# Configuration Guide

## Configuration File Format

The Cache Simulator uses JSON configuration files for advanced settings.

## Basic Configuration

### Minimal Configuration
```json
{
  "l1": {
    "size": 32768,
    "associativity": 4,
    "blockSize": 64
  }
}
```

### Complete Configuration
```json
{
  "l1": {
    "size": 32768,
    "associativity": 4,
    "blockSize": 64,
    "replacementPolicy": "LRU",
    "writePolicy": "WriteBack",
    "prefetching": {
      "enabled": true,
      "distance": 4,
      "adaptive": true
    }
  },
  "l2": {
    "size": 262144,
    "associativity": 8,
    "blockSize": 64,
    "replacementPolicy": "LRU",
    "writePolicy": "WriteBack"
  },
  "victimCache": {
    "enabled": false,
    "size": 4
  },
  "multiprocessor": {
    "enabled": false,
    "numProcessors": 1,
    "coherenceProtocol": "MESI",
    "interconnect": "Bus"
  }
}
```

## Configuration Options

### Cache Configuration

#### L1 Cache
- **size**: Cache size in bytes (default: 32768)
- **associativity**: Set associativity (default: 4)
- **blockSize**: Block size in bytes (default: 64)
- **replacementPolicy**: LRU, FIFO, Random, PLRU, NRU (default: LRU)
- **writePolicy**: WriteBack, WriteThrough (default: WriteBack)

#### L2 Cache
- **size**: Cache size in bytes (default: 262144)
- **associativity**: Set associativity (default: 8)
- **blockSize**: Block size in bytes (default: 64)
- **replacementPolicy**: LRU, FIFO, Random, PLRU, NRU (default: LRU)
- **writePolicy**: WriteBack, WriteThrough (default: WriteBack)

### Advanced Features

#### Victim Cache
```json
"victimCache": {
  "enabled": true,
  "size": 8,
  "replacementPolicy": "FIFO"
}
```

#### Prefetching
```json
"prefetching": {
  "enabled": true,
  "distance": 4,
  "adaptive": true,
  "stridePrediction": true
}
```

#### Multi-processor Simulation
```json
"multiprocessor": {
  "enabled": true,
  "numProcessors": 4,
  "coherenceProtocol": "MESI",
  "interconnect": "Crossbar",
  "interconnectLatency": 10
}
```

## Example Configurations

### High-Performance Configuration
```json
{
  "l1": {
    "size": 65536,
    "associativity": 8,
    "blockSize": 64,
    "replacementPolicy": "LRU"
  },
  "l2": {
    "size": 1048576,
    "associativity": 16,
    "blockSize": 64
  },
  "victimCache": {
    "enabled": true,
    "size": 16
  }
}
```

### Low-Power Configuration
```json
{
  "l1": {
    "size": 16384,
    "associativity": 2,
    "blockSize": 32,
    "replacementPolicy": "FIFO"
  },
  "l2": {
    "size": 131072,
    "associativity": 4,
    "blockSize": 32
  },
  "prefetching": {
    "enabled": false
  }
}
```

### Multi-processor Configuration
```json
{
  "l1": {
    "size": 32768,
    "associativity": 4,
    "blockSize": 64,
    "replacementPolicy": "NRU"
  },
  "multiprocessor": {
    "enabled": true,
    "numProcessors": 4,
    "coherenceProtocol": "MESI",
    "interconnect": "Mesh",
    "interconnectLatency": 5
  }
}
```

## Validation

The simulator validates all configuration parameters and provides detailed error messages for invalid settings:

- Cache sizes must be powers of 2
- Block sizes must be powers of 2
- Associativity must be a power of 2 and ≤ cache_size/block_size
- Prefetch distance must be positive
- Number of processors must be between 1 and 64