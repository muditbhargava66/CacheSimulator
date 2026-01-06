# Contributing to Cache Simulator

## Development Setup

### Prerequisites
- C++17 compatible compiler
- CMake 3.14+
- Git
- Doxygen (for documentation)

### Setting up Development Environment

1. **Fork and clone the repository:**
   ```bash
   git clone https://github.com/yourusername/CacheSimulator.git
   cd CacheSimulator
   ```

2. **Create development build:**
   ```bash
   mkdir build-dev && cd build-dev
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make -j$(nproc)
   ```

3. **Run tests:**
   ```bash
   ctest --verbose
   ```

## Code Style Guidelines

### C++ Standards
- Use C++17 features appropriately
- Follow RAII principles
- Use smart pointers for memory management
- Prefer const-correctness
- Use meaningful variable and function names

### Formatting
- Use 4 spaces for indentation
- Maximum line length: 100 characters
- Place opening braces on the same line
- Use camelCase for variables and functions
- Use PascalCase for classes and types

### Example:
```cpp
class CacheSimulator {
public:
    explicit CacheSimulator(const Config& config);
    
    bool processAccess(uint64_t address, bool isWrite);
    
private:
    std::unique_ptr<Cache> l1Cache_;
    std::unique_ptr<Cache> l2Cache_;
    Statistics stats_;
};
```

## Testing Guidelines

### Unit Tests
- Write tests for all new functionality
- Use descriptive test names
- Test both success and failure cases
- Aim for high code coverage

### Test Structure
```cpp
class MyFeatureTest {
public:
    static void testBasicFunctionality() {
        // Arrange
        MyFeature feature(config);
        
        // Act
        bool result = feature.doSomething();
        
        // Assert
        assert(result == true);
    }
};
```

### Integration Tests
- Test complete workflows
- Use realistic trace files
- Verify performance characteristics
- Test multi-threading scenarios

## Submission Process

### Before Submitting
1. **Run all tests:**
   ```bash
   cd build-dev
   ctest
   ```

2. **Check code formatting:**
   ```bash
   # Format code if needed
   clang-format -i src/**/*.cpp src/**/*.h
   ```

3. **Update documentation:**
   - Update relevant .md files
   - Add Doxygen comments for new APIs
   - Update CHANGELOG.md

### Pull Request Guidelines

1. **Create feature branch:**
   ```bash
   git checkout -b feature/my-new-feature
   ```

2. **Make atomic commits:**
   ```bash
   git commit -m "Add NRU replacement policy implementation"
   git commit -m "Add unit tests for NRU policy"
   git commit -m "Update documentation for NRU policy"
   ```

3. **Write descriptive PR description:**
   - What does this PR do?
   - Why is this change needed?
   - How was it tested?
   - Any breaking changes?

### Commit Message Format
```
type(scope): brief description

Detailed explanation of the change, including:
- What was changed
- Why it was changed
- Any side effects or considerations

Fixes #123
```

Types: feat, fix, docs, style, refactor, test, chore

## Architecture Guidelines

### Adding New Features

1. **Design first:**
   - Document the design in docs/developer/
   - Consider backward compatibility
   - Plan for testability

2. **Implementation:**
   - Follow existing patterns
   - Add comprehensive error handling
   - Include performance considerations

3. **Testing:**
   - Unit tests for individual components
   - Integration tests for complete features
   - Performance benchmarks if applicable

### Code Organization

```
src/
├── core/           # Core simulation engine
├── utils/          # Utility classes and functions
└── tools/          # Command-line tools

tests/
├── unit/           # Unit tests
├── integration/    # Integration tests
└── performance/    # Performance benchmarks
```

## Review Process

### What We Look For
- Code quality and style compliance
- Comprehensive testing
- Clear documentation
- Performance impact assessment
- Backward compatibility

### Review Timeline
- Initial review within 48 hours
- Follow-up reviews within 24 hours
- Merge after approval from maintainers

## Getting Help

- **Issues:** Use GitHub issues for bugs and feature requests
- **Discussions:** Use GitHub discussions for questions
- **Email:** Contact maintainers for sensitive issues

## Recognition

Contributors are recognized in:
- CHANGELOG.md for their contributions
- README.md contributors section
- Git commit history

Thank you for contributing to Cache Simulator!