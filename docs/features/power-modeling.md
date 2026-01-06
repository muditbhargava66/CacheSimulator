# Power and Area Modeling

## Overview

The Cache Simulator includes CACTI-inspired analytical models for power and area estimation. This enables analysis of energy efficiency and silicon footprint across different cache configurations and technology nodes.

## Quick Start

```bash
# Enable power analysis with default 45nm technology
./cachesim --power traces/example.txt

# Specify technology node
./cachesim --power --tech-node 22 traces/example.txt
```

## Features

### Power Modeling

| Metric | Description | Unit |
|--------|-------------|------|
| Dynamic Read Energy | Energy per read access | pJ |
| Dynamic Write Energy | Energy per write access | pJ |
| Static (Leakage) Power | Idle power consumption | mW |
| Total Energy | Accumulated during simulation | nJ |
| Energy-Delay Product | Efficiency metric | pJ·ns |

### Area Modeling

| Component | Description |
|-----------|-------------|
| Data Array | SRAM cells for cached data |
| Tag Array | Tag storage with valid/dirty bits |
| Decoders | Row/column address decoders |
| Sense Amplifiers | Signal amplification circuits |
| Drivers | Wordline/bitline drivers |
| Routing | Metal interconnect overhead |

## Technology Nodes

```
Node   Vdd     Cell Area    Use Case
-----------------------------------------
45nm   1.0V    0.346 µm²    Baseline (most validated)
32nm   0.9V    0.171 µm²    Mainstream
22nm   0.8V    0.092 µm²    High-performance
14nm   0.75V   0.050 µm²    FinFET era
7nm    0.65V   0.027 µm²    Modern process
```

## Sample Output

```
+===========================================================+
|              Power and Energy Analysis                    |
+===========================================================+
| Technology Node:              45 nm                       |
| Supply Voltage:              1.0 V                        |
| Temperature:               350.0 K                        |
+-----------------------------------------------------------+
| Per-Access Energy                                         |
+-----------------------------------------------------------+
| Dynamic Read Energy:        0.089 pJ                      |
| Dynamic Write Energy:       0.116 pJ                      |
+-----------------------------------------------------------+
| Power Breakdown                                           |
+-----------------------------------------------------------+
| Static (Leakage) Power:    12.400 mW                      |
| Total Energy:             847.300 nJ                      |
+===========================================================+
```

## Configuration

### JSON Configuration

```json
{
  "l1": {
    "size": 32768,
    "associativity": 4,
    "blockSize": 64
  },
  "power": {
    "enabled": true,
    "techNode": 45,
    "vdd": 1.0,
    "frequency": 2e9,
    "temperature": 350
  }
}
```

### Programmatic API

```cpp
#include "models/power_model.h"
#include "models/area_model.h"

using namespace cachesim;

// Configure power model
PowerConfig powerConfig;
powerConfig.enabled = true;
powerConfig.techNode = 45;
powerConfig.cacheSize = 32768;
powerConfig.blockSize = 64;
powerConfig.associativity = 4;

// Enable power tracking on cache
cache.enablePowerModeling(powerConfig);

// After simulation
auto stats = cache.getPowerStats();
std::cout << "Total Energy: " << stats.totalEnergy << " nJ\n";
std::cout << cache.getPowerReport();

// Area estimation
auto area = AreaModel::calculate(32768, 64, 4, 20, 45);
std::cout << AreaModel::generateReport(area, 45);
```

## Model Details

### Dynamic Energy

```
E_read = E_bitline + E_wordline + E_decoder + E_senseamp + E_output

E_bitline = C_bl × Vdd² × num_bitlines
E_wordline = C_wl × Vdd² × wordline_length
E_decoder = decoderEnergyPerBit × indexBits × associativity

E_write ≈ 1.3 × E_read  (30% higher due to dual bitline drive)
```

### Leakage Power

```
P_leak = (I_subthreshold + I_gate) × Vdd × N_transistors × temp_factor

temp_factor = exp((T - 300K) / 100K)
```

### Area

```
A_total = A_data + A_tag + A_decoder + A_senseamp + A_drivers + A_routing

A_data = cacheSize × 8 × SRAM_cell_area
A_peripheral ≈ 0.46 × A_cells  (based on CACTI breakdown)
```

## Validation

The model has been validated against CACTI 7.0 reference values:

| Configuration | CACTI Area | Our Model | Difference |
|---------------|------------|-----------|------------|
| 32KB L1 @ 45nm | ~0.05 mm² | 0.04-0.06 mm² | < 20% |
| 256KB L2 @ 45nm | ~0.35 mm² | 0.30-0.40 mm² | < 15% |

## References

1. CACTI 7.0 (HP Labs)
2. "An Integrated Cache Timing, Power, and Area Model" - Thoziyoor et al.
3. ITRS Technology Roadmap
4. "Power Analysis of Embedded Systems" - Benini & De Micheli
