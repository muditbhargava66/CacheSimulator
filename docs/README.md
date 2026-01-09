# Cache Simulator Documentation

Comprehensive documentation for the Cache Simulator.

## Documentation Structure

### User Documentation

| Document                                   | Description                              |
| ------------------------------------------ | ---------------------------------------- |
| [Getting Started](user/getting-started.md) | Installation and basic usage             |
| [User Guide](user/user-guide.md)           | Complete user manual                     |
| [Configuration](user/configuration.md)     | Configuration options and examples       |
| [Config Parsing](user/config-parsing.md)   | Config file format and key normalization |
| [CLI Reference](user/cli-reference.md)     | All command-line options                 |
| [Examples](user/examples.md)               | Usage examples and tutorials             |
| [Analysis Tools](user/analysis.md)         | Performance analysis and profiling       |

### Developer Documentation

| Document                                    | Description                         |
| ------------------------------------------- | ----------------------------------- |
| [Architecture](developer/architecture.md)   | System design and components        |
| [Building](developer/building.md)           | Build instructions and requirements |
| [API Reference](developer/api-reference.md) | Code API documentation              |

### Feature Documentation

| Document                                                   | Description                                 |
| ---------------------------------------------------------- | ------------------------------------------- |
| [L3 Cache](features/l3-cache.md)                           | Third-level cache with inclusive policy     |
| [Coherence Protocols](features/coherence-protocols.md)     | MSI, MESI, MOESI protocol implementations   |
| [Interconnects](features/interconnects.md)                 | Bus, Crossbar, Mesh, Ring, Torus topologies |
| [Power Modeling](features/power-modeling.md)               | CACTI-inspired power and area analysis      |
| [Replacement Policies](features/replacement-policies.md)   | LRU, FIFO, NRU, PLRU, Random                |
| [Victim Cache](features/victim-cache.md)                   | Victim cache implementation                 |
| [Multiprocessor](features/multiprocessor.md)               | Multi-core simulation and coherence         |
| [Prefetching](features/prefetching.md)                     | Stream buffer and stride prefetching        |
| [Side-Channel Research](features/side-channel-research.md) | Cache timing attack simulation and analysis |

### Platform Documentation

| Document              | Description                             |
| --------------------- | --------------------------------------- |
| [Windows](WINDOWS.md) | Windows build guide and troubleshooting |

## Quick Links

- [Installation Guide](user/getting-started.md#installation)
- [Basic Usage](user/getting-started.md#basic-usage)
- [Configuration Examples](user/configuration.md#example-configurations)
- [Changelog](../CHANGELOG.md)
- [Contributing](../contributing.md)
