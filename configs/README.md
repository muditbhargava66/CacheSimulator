# Cache Simulator Configuration Files

This directory contains pre-configured JSON configuration files for various simulation scenarios.

## Available Configurations

### General Purpose
- **full_features.json** - Configuration with all features enabled
- **high_performance.json** - Optimized for maximum performance

### Specialized Configurations
- **nru_optimized.json** - Configuration optimized for NRU replacement policy
- **victim_cache_config.json** - Configuration with victim cache enabled
- **write_optimized.json** - Configuration optimized for write operations
- **write_intensive.json** - Configuration for write-intensive workloads

### Multi-processor Configurations
- **multiprocessor_4core.json** - 4-core processor configuration
- **multiprocessor_system.json** - General multi-processor system configuration

## Configuration Format

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

## Usage

```bash
# Run with specific configuration
./bin/cachesim --config configs/high_performance.json traces/trace1.txt

# Run with victim cache configuration
./bin/cachesim --config configs/victim_cache_config.json traces/conflict_heavy.txt

# Run with multi-processor configuration
./bin/cachesim --config configs/multiprocessor_4core.json traces/multiprocessor_coherence.txt
```

## Creating Custom Configurations

You can create your own configuration files by copying and modifying any of the existing ones. The simulator validates all configuration parameters and provides detailed error messages for invalid settings.

### Key Parameters

#### Cache Configuration
- **size**: Cache size in bytes (must be power of 2)
- **associativity**: Set associativity (must be power of 2)
- **blockSize**: Block size in bytes (must be power of 2)
- **replacementPolicy**: LRU, FIFO, Random, PLRU, NRU
- **writePolicy**: WriteBack, WriteThrough

#### Victim Cache
- **enabled**: true/false
- **size**: Number of entries

#### Multi-processor
- **enabled**: true/false
- **numProcessors**: Number of processor cores
- **coherenceProtocol**: MESI
- **interconnect**: Bus, Crossbar, Mesh