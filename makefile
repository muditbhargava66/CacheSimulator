# Cache Simulator Makefile
# This Makefile provides a simple alternative to CMake

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
CXXFLAGS_DEBUG = -std=c++17 -Wall -Wextra -Wpedantic -O0 -g3 -DDEBUG

# Directories
SRCDIR = src
BUILDDIR = build
BINDIR = $(BUILDDIR)/bin
OBJDIR = $(BUILDDIR)/obj
TESTDIR = tests
TOOLSDIR = tools
TRACESDIR = traces
SCRIPTSDIR = scripts

# Include paths
INCLUDES = -I$(SRCDIR) -I$(SRCDIR)/core -I$(SRCDIR)/utils

# Source files
CORE_SOURCES = $(wildcard $(SRCDIR)/core/*.cpp)
UTILS_SOURCES = $(wildcard $(SRCDIR)/utils/*.cpp)
MAIN_SOURCE = $(SRCDIR)/main.cpp
ALL_SOURCES = $(CORE_SOURCES) $(UTILS_SOURCES) $(MAIN_SOURCE)

# Object files
CORE_OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(CORE_SOURCES))
UTILS_OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(UTILS_SOURCES))
MAIN_OBJECT = $(OBJDIR)/main.o
ALL_OBJECTS = $(CORE_OBJECTS) $(UTILS_OBJECTS) $(MAIN_OBJECT)

# Dependencies
DEPS = $(patsubst %.o,%.d,$(ALL_OBJECTS))

# Test sources and objects
UNIT_TEST_SOURCES = $(wildcard $(TESTDIR)/unit/*.cpp)
VALIDATION_TEST_SOURCES = $(wildcard $(TESTDIR)/validation/*.cpp)
UNIT_TEST_OBJECTS = $(patsubst $(TESTDIR)/%.cpp,$(OBJDIR)/tests/%.o,$(UNIT_TEST_SOURCES))
VALIDATION_TEST_OBJECTS = $(patsubst $(TESTDIR)/%.cpp,$(OBJDIR)/tests/%.o,$(VALIDATION_TEST_SOURCES))
UNIT_TEST_BINS = $(patsubst $(TESTDIR)/unit/%.cpp,$(BINDIR)/tests/unit/%,$(UNIT_TEST_SOURCES))
VALIDATION_TEST_BINS = $(patsubst $(TESTDIR)/validation/%.cpp,$(BINDIR)/tests/validation/%,$(VALIDATION_TEST_SOURCES))

# Tool sources and objects
TOOL_SOURCES = $(wildcard $(TOOLSDIR)/*.cpp)
TOOL_OBJECTS = $(patsubst $(TOOLSDIR)/%.cpp,$(OBJDIR)/tools/%.o,$(TOOL_SOURCES))
TOOL_BINS = $(patsubst $(TOOLSDIR)/%.cpp,$(BINDIR)/tools/%,$(TOOL_SOURCES))

# Library for tests and tools
LIB_NAME = libcachesim.a
LIB = $(BUILDDIR)/$(LIB_NAME)

# Main targets
.PHONY: all clean debug test tools run run_tests run_tools install help docs format

all: $(BINDIR)/cachesim tools

debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: all

# Create build directories
$(OBJDIR)/core $(OBJDIR)/utils $(BINDIR) $(OBJDIR)/tests/unit $(OBJDIR)/tests/validation $(OBJDIR)/tools $(BINDIR)/tests/unit $(BINDIR)/tests/validation $(BINDIR)/tools:
	mkdir -p $@

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)/core $(OBJDIR)/utils
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Compile test files
$(OBJDIR)/tests/%.o: $(TESTDIR)/%.cpp | $(OBJDIR)/tests/unit $(OBJDIR)/tests/validation
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Compile tool files
$(OBJDIR)/tools/%.o: $(TOOLSDIR)/%.cpp | $(OBJDIR)/tools
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Create static library
$(LIB): $(CORE_OBJECTS) $(UTILS_OBJECTS)
	ar rcs $@ $^

# Link main executable
$(BINDIR)/cachesim: $(MAIN_OBJECT) $(LIB) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $< -L$(BUILDDIR) -lcachesim -o $@

# Link unit test executables
$(BINDIR)/tests/unit/%: $(OBJDIR)/tests/unit/%.o $(LIB) | $(BINDIR)/tests/unit
	$(CXX) $(CXXFLAGS) $< -L$(BUILDDIR) -lcachesim -o $@

# Link validation test executables
$(BINDIR)/tests/validation/%: $(OBJDIR)/tests/validation/%.o $(LIB) | $(BINDIR)/tests/validation
	$(CXX) $(CXXFLAGS) $< -L$(BUILDDIR) -lcachesim -o $@

# Link tool executables
$(BINDIR)/tools/%: $(OBJDIR)/tools/%.o $(LIB) | $(BINDIR)/tools
	$(CXX) $(CXXFLAGS) $< -L$(BUILDDIR) -lcachesim -o $@

# Build unit tests
unit_tests: $(UNIT_TEST_BINS)

# Build validation tests
validation_tests: $(VALIDATION_TEST_BINS)

# Build all tests
test: unit_tests validation_tests

# Build all tools
tools: $(TOOL_BINS)

# Run the simulator
run: $(BINDIR)/cachesim
	./$(BINDIR)/cachesim 64 32768 4 262144 8 1 4 $(TRACESDIR)/trace1.txt

# Run unit tests
run_unit_tests: unit_tests
	@for test in $(UNIT_TEST_BINS); do \
		echo "Running $$test..."; \
		$$test; \
	done

# Run validation tests
run_validation_tests: validation_tests
	@for test in $(VALIDATION_TEST_BINS); do \
		echo "Running $$test..."; \
		$$test; \
	done

# Run all tests
run_tests: run_unit_tests run_validation_tests

# Run tools
run_tools: tools
	@for tool in $(TOOL_BINS); do \
		echo "Running $$tool --help..."; \
		$$tool --help; \
	done

# Run a specific simulation script
run_script: $(BINDIR)/cachesim tools
	bash $(SCRIPTSDIR)/run_simulations.sh

# Install the simulator
install: all
	mkdir -p $(DESTDIR)/usr/local/bin
	mkdir -p $(DESTDIR)/usr/local/share/cachesim/traces
	mkdir -p $(DESTDIR)/usr/local/share/cachesim/scripts
	install -m 755 $(BINDIR)/cachesim $(DESTDIR)/usr/local/bin/
	install -m 755 $(BINDIR)/tools/* $(DESTDIR)/usr/local/bin/
	install -m 644 $(TRACESDIR)/* $(DESTDIR)/usr/local/share/cachesim/traces/
	install -m 755 $(SCRIPTSDIR)/* $(DESTDIR)/usr/local/share/cachesim/scripts/

# Remove build files
clean:
	rm -rf $(BUILDDIR)

# Generate documentation with Doxygen
docs:
	@if command -v doxygen >/dev/null 2>&1; then \
		mkdir -p docs/generated; \
		doxygen docs/Doxyfile; \
	else \
		echo "Doxygen not found. Please install doxygen to generate documentation."; \
	fi

# Format source code with clang-format
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		find $(SRCDIR) $(TESTDIR) $(TOOLSDIR) -name "*.cpp" -o -name "*.h" | xargs clang-format -i -style=file; \
		echo "Code formatting complete."; \
	else \
		echo "clang-format not found. Please install clang-format to format code."; \
	fi

# Show help
help:
	@echo "Cache Simulator Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all             - Build main executable and tools"
	@echo "  debug           - Build with debug symbols"
	@echo "  test            - Build all tests"
	@echo "  tools           - Build utility tools"
	@echo "  clean           - Remove build files"
	@echo "  run             - Run simulator with default parameters"
	@echo "  run_tests       - Build and run all tests"
	@echo "  run_script      - Run the simulation script"
	@echo "  install         - Install to /usr/local"
	@echo "  docs            - Generate documentation with Doxygen"
	@echo "  format          - Format source code with clang-format"
	@echo "  help            - Show this help message"
	@echo ""
	@echo "Example:"
	@echo "  make run TRACESDIR=my_traces"

# Include dependencies
-include $(DEPS)