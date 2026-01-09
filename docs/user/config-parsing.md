# Configuration File Parsing

This document describes the configuration file format and parsing behavior for Cache Simulator v1.4.1+.

## Key Naming Convention

The configuration parser supports **both snake_case and camelCase** key names for backward compatibility.

### Supported Formats

```json
// snake_case (v1.2.x style)
{
  "block_size": 64,
  "write_policy": "WriteBack",
  "replacement_policy": "LRU"
}

// camelCase (preferred)
{
  "blockSize": 64,
  "writePolicy": "WriteBack",
  "replacementPolicy": "LRU"
}
```

Both formats are automatically normalized to camelCase during parsing.

## Key Normalization

The parser converts snake_case keys to camelCase using the `normalizeConfigKey()` function:

| Input Key            | Normalized Key      |
| -------------------- | ------------------- |
| `block_size`         | `blockSize`         |
| `write_policy`       | `writePolicy`       |
| `replacement_policy` | `replacementPolicy` |
| `associativity`      | `associativity`     |

## Recognized Keys

The parser validates keys against a known set:

### Cache Configuration
- `size`, `associativity`, `blockSize`, `replacementPolicy`, `writePolicy`

### Stream Buffer / Prefetching
- `enabled`, `distance`, `degree`, `adaptive`, `forLevel`

### Victim Cache
- `entries`

### Profiler Regions (v1.4.1)
- `startAddress`, `endAddress`, `name`

## Error Messages

Unrecognized keys generate warnings with suggestions:

```
[Config Warning] Unrecognized key 'blok_size' in section 'l1'. Did you mean 'blockSize'?
```

## Config File Version

All example config files should specify version `"1.4.0"` or later:

```json
{
  "name": "My Configuration",
  "version": "1.4.0",
  "l1": { ... }
}
```

## Profiler Regions (v1.4.1)

User-defined memory regions for profiling:

```json
{
  "profilerRegions": [
    {
      "startAddress": "0x1000",
      "endAddress": "0x2000",
      "name": "Stack"
    },
    {
      "startAddress": "0x10000",
      "endAddress": "0x20000",
      "name": "Heap"
    }
  ]
}
```

## Testing

Config parsing is validated by integration tests:
- `tests/integration/config_parsing_test.cpp`
- Tests all 8 example JSON configs
- Verifies snake_case normalization
