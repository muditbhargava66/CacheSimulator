# Command Line Reference

## Synopsis

```bash
cachesim [OPTIONS] <trace_file>
```

## Options

### General Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message and exit |
| `-v, --version` | Show version information |
| `--verbose` | Enable verbose output |
| `--no-color` | Disable colored output |

### Configuration Options

| Option | Description |
|--------|-------------|
| `-c, --config <file>` | Specify configuration file (JSON format) |

### Simulation Options

| Option | Description |
|--------|-------------|
| `--victim-cache` | Enable victim cache |
| `-p, --parallel [threads]` | Enable parallel processing (optional thread count) |

### Output Options

| Option | Description |
|--------|-------------|
| `--vis, --visualize` | Visualize cache behavior |
| `--charts` | Show statistical charts |
| `-e, --export [file]` | Export results to CSV file |

### Analysis Options

| Option | Description |
|--------|-------------|
| `-b, --benchmark` | Run performance benchmark |

## Examples

### Basic Simulation
```bash
cachesim trace.txt
```

### Advanced Configuration
```bash
cachesim --config advanced.json --victim-cache --parallel 4 trace.txt
```

### Visualization and Export
```bash
cachesim --visualize --charts --export results.csv trace.txt
```

### Benchmarking
```bash
cachesim --benchmark --parallel trace.txt
```

## Default Configuration

When no configuration file is specified:
- **Block Size:** 64 bytes
- **L1 Cache:** 32KB, 4-way associative
- **L2 Cache:** 256KB, 8-way associative
- **Replacement Policy:** LRU
- **Prefetching:** Enabled (distance=4)

## Trace File Format

### Simple Format
```
# Comments start with #
R 0x1000    # Read from address 0x1000
W 0x1004    # Write to address 0x1004
```

### JSON Format
```json
{
  "accesses": [
    {"address": "0x1000", "type": "read"},
    {"address": "0x1004", "type": "write"}
  ]
}
```

## Exit Codes

| Code | Description |
|------|-------------|
| 0 | Success |
| 1 | Invalid arguments or configuration |
| 2 | File not found or I/O error |
| 3 | Simulation error |