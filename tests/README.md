# Cache Simulator Test Suite

This directory contains the comprehensive test suite for the Cache Simulator v1.2.0.

## Test Organization

### Unit Tests (`tests/unit/`)
Unit tests for individual components and modules.

#### Core Components (`tests/unit/core/`)
- **cache_test.cpp** - Basic cache functionality tests
- **memory_hierarchy_test.cpp** - Memory hierarchy integration tests
- **stream_buffer_test.cpp** - Stream buffer and prefetching tests
- **victim_cache_test.cpp** - Victim cache implementation tests

#### Policies (`tests/unit/policies/`)
- **nru_policy_test.cpp** - NRU replacement policy tests
- **write_policy_test.cpp** - Write policy framework tests

#### Utilities (`tests/unit/utils/`)
- **parallel_processing_test.cpp** - Parallel execution framework tests
- **visualization_test.cpp** - Statistical visualization tests

### Integration Tests (`tests/integration/`)
End-to-end tests that verify complete system functionality.

- **integration_test_v1_2_0.cpp** - Comprehensive v1.2.0 feature integration tests
- **known_traces.cpp** - Validation tests with known trace patterns

### Performance Tests (`tests/performance/`)
Performance benchmarks and stress tests.

## Running Tests

### All Tests
```bash
cd build
ctest --verbose
```

### Specific Test Categories
```bash
# Unit tests only
ctest -R "unit"

# Integration tests only
ctest -R "integration"

# Core component tests
ctest -R "cache_test|memory_hierarchy_test"

# Policy tests
ctest -R "policy_test"
```

### Individual Tests
```bash
# Run specific test executable
./bin/tests/unit/core/cache_test
./bin/tests/unit/policies/nru_policy_test
./bin/tests/integration/integration_test_v1_2_0
```

## Test Development Guidelines

### Unit Test Structure
```cpp
class ComponentTest {
public:
    static void testBasicFunctionality() {
        // Test implementation
    }
    
    static void testEdgeCases() {
        // Edge case testing
    }
    
    static void runAllTests() {
        testBasicFunctionality();
        testEdgeCases();
        std::cout << "All Component tests passed!" << std::endl;
    }
};

int main() {
    ComponentTest::runAllTests();
    return 0;
}
```

### Integration Test Structure
```cpp
class IntegrationTest {
public:
    static void testCompleteWorkflow() {
        // End-to-end workflow testing
    }
    
    static void testPerformanceCharacteristics() {
        // Performance validation
    }
};
```

### Test Naming Conventions
- Test files: `component_test.cpp`
- Test classes: `ComponentTest`
- Test methods: `testSpecificFunctionality()`
- Test executables: `component_test`

## Test Coverage

The test suite covers:
- ✅ Core cache functionality
- ✅ Memory hierarchy operations
- ✅ Replacement policies (LRU, FIFO, Random, PLRU, NRU)
- ✅ Write policies (WriteBack, WriteThrough, No-Write-Allocate)
- ✅ Victim cache implementation
- ✅ Parallel processing framework
- ✅ Statistical visualization
- ✅ Multi-processor simulation
- ✅ Configuration validation
- ✅ Trace parsing and processing

## Adding New Tests

1. **Choose appropriate category:**
   - Unit tests for individual components
   - Integration tests for complete workflows
   - Performance tests for benchmarking

2. **Follow naming conventions:**
   - Place in correct subdirectory
   - Use descriptive filenames
   - Follow existing patterns

3. **Update CMakeLists.txt:**
   - Add new test executable
   - Register with CTest

4. **Document test purpose:**
   - Add comments explaining what is tested
   - Include expected behavior
   - Note any special requirements

## Continuous Integration

Tests are automatically run on:
- Pull requests
- Main branch commits
- Release tags

All tests must pass before code can be merged.