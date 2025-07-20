# Cache Simulator Scripts

This directory contains utility scripts for building, testing, and running the Cache Simulator.

## Available Scripts

### Build Script
**File**: `build_all.sh`

Comprehensive build script with various options.

```bash
# Basic build (Release mode)
./scripts/build_all.sh

# Debug build
./scripts/build_all.sh --debug

# Clean build
./scripts/build_all.sh --clean

# Build without tests
./scripts/build_all.sh --no-tests

# Build with documentation
./scripts/build_all.sh --docs

# Specify number of parallel jobs
./scripts/build_all.sh --jobs=4
```

### Benchmark Script
**File**: `run_benchmarks.sh`

Runs a comprehensive set of benchmarks with different configurations and traces.

```bash
# Run all benchmarks
./scripts/run_benchmarks.sh
```

The script will run various benchmarks and save the results in the `benchmark_results` directory.

### Simulation Script
**File**: `run_simulations.sh`

Comprehensive simulation runner with parallel execution and detailed reporting.

```bash
# Run with default settings
./scripts/run_simulations.sh

# Run with verbose output and chart generation
./scripts/run_simulations.sh --verbose --charts

# Specify custom paths and parallel jobs
./scripts/run_simulations.sh --simulator=./build/bin/cachesim --traces=./traces --results=./my_results --jobs=8

# Get help
./scripts/run_simulations.sh --help
```

### Trace Validation Script
**File**: `validate_traces.sh`

Validates trace file format and content for correctness.

```bash
# Validate all traces
./scripts/validate_traces.sh

# Verbose validation with report generation
./scripts/validate_traces.sh --verbose --report

# Validate custom trace directory
./scripts/validate_traces.sh --traces=./custom_traces --fix

# Get help
./scripts/validate_traces.sh --help
```

### Release Creation Script
**File**: `create_release.sh`

Creates a release package with binaries and documentation.

```bash
# Create release with detected version
./scripts/create_release.sh

# Specify version
./scripts/create_release.sh --version=1.2.0

# Specify output directory
./scripts/create_release.sh --output=releases
```

The script will create a release package in the specified output directory.

## Usage

Make sure to make the scripts executable before running them:

```bash
chmod +x scripts/*.sh
```

All scripts should be run from the project root directory.

## Script Features

### Build Script Features
- Support for Debug and Release builds
- Optional test building
- Documentation generation
- Parallel compilation
- Clean build option

### Benchmark Script Features
- Automated benchmark execution
- Multiple configuration testing
- Result collection and organization
- Performance metric extraction

### Simulation Script Features
- Colorized output
- Multiple configuration support
- CSV result export
- Automatic chart generation (if gnuplot available)
- Configurable paths

### Release Script Features
- Cross-platform support
- Automatic version detection
- Complete package creation
- Multiple archive formats

## Creating New Scripts

When creating new scripts:

1. Add a shebang line: `#!/bin/bash`
2. Include descriptive comments
3. Add error handling
4. Make the script executable: `chmod +x scripts/new_script.sh`
5. Update this README.md with usage instructions

## Examples

### Complete Build and Test Workflow
```bash
# Clean build with tests
./scripts/build_all.sh --clean

# Run comprehensive benchmarks
./scripts/run_benchmarks.sh

# Run detailed simulations
./scripts/run_simulations.sh

# Create release package
./scripts/create_release.sh
```

### Development Workflow
```bash
# Debug build for development
./scripts/build_all.sh --debug --docs

# Quick benchmark check
./scripts/run_benchmarks.sh

# Generate test traces
./build/bin/tools/trace_generator --generate-all test_traces/
```