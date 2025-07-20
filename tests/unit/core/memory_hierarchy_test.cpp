#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <optional>
#include <string_view>
#include <algorithm>

#include "../../src/core/memory_hierarchy.h"
#include "../../src/utils/trace_parser.h"

using namespace cachesim;
namespace fs = std::filesystem;

class MemoryHierarchyTest {
public:
    // Create a temporary trace file with the given access pattern
    static std::string createTestTraceFile(const std::vector<std::pair<bool, uint32_t>>& accesses) {
        // Create a temporary file
        std::string tempFilename = "temp_trace_" + std::to_string(std::rand()) + ".txt";
        std::ofstream tempFile(tempFilename);
        
        if (!tempFile) {
            throw std::runtime_error("Failed to create temporary trace file");
        }
        
        // Write accesses to the file
        for (const auto& [isWrite, address] : accesses) {
            tempFile << (isWrite ? "w " : "r ") << "0x" << std::hex << address << std::endl;
        }
        
        tempFile.close();
        return tempFilename;
    }
    
    // Test basic initialization
    static void testInitialization() {
        std::cout << "Running Memory Hierarchy initialization test..." << std::endl;
        
        // Create configuration
        MemoryHierarchyConfig config;
        
        // L1 configuration: 1KB size, 2-way, 64B blocks, no prefetching
        config.l1Config = {1024, 2, 64, false, 0};
        
        // L2 configuration: 4KB size, 4-way, 64B blocks, no prefetching
        config.l2Config = CacheConfig{4096, 4, 64, false, 0};
        
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Verify initial state (not much to check directly, mainly ensuring no exceptions)
        assert(hierarchy.getL1Misses() == 0 && "Initial L1 misses should be 0");
        assert(hierarchy.getL1Reads() == 0 && "Initial L1 reads should be 0");
        assert(hierarchy.getL1Writes() == 0 && "Initial L1 writes should be 0");
        
        std::cout << "Memory Hierarchy initialization test passed." << std::endl;
    }
    
    // Test basic access patterns
    static void testAccessPatterns() {
        std::cout << "Running Memory Hierarchy access patterns test..." << std::endl;
        
        // Create configuration for a small cache to ensure some misses
        MemoryHierarchyConfig config;
        config.l1Config = {256, 2, 64, false, 0}; // 256 bytes, 2-way, 64B blocks
        
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Create accesses to test various patterns
        std::vector<std::pair<bool, uint32_t>> accesses = {
            {false, 0x1000},  // Read, should miss
            {false, 0x1000},  // Read, should hit
            {true, 0x2000},   // Write, should miss
            {false, 0x2000},  // Read, should hit
            {false, 0x3000},  // Read, should miss
            {false, 0x1000},  // Read, should hit (still in cache)
            {false, 0x4000},  // Read, should miss and may evict something
            {false, 0x5000},  // Read, should miss and may evict something
            {false, 0x1000}   // Read, may hit or miss depending on replacement
        };
        
        // Apply accesses
        for (const auto& [isWrite, address] : accesses) {
            hierarchy.access(address, isWrite);
        }
        
        // Check basic metrics - exact values depend on replacement policy
        assert(hierarchy.getL1Reads() == 7 && "Should have 7 reads");
        assert(hierarchy.getL1Writes() == 1 && "Should have 1 write");
        assert(hierarchy.getL1Misses() > 0 && "Should have some misses");
        assert(hierarchy.getL1Misses() < 9 && "Should have fewer misses than accesses");
        
        std::cout << "Memory Hierarchy access patterns test passed." << std::endl;
    }
    
    // Test L1 to L2 interactions
    static void testL1L2Interactions() {
        std::cout << "Running Memory Hierarchy L1-L2 interactions test..." << std::endl;
        
        // Create configuration with both L1 and L2
        MemoryHierarchyConfig config;
        config.l1Config = {256, 2, 64, false, 0};    // 256 bytes, 2-way, 64B blocks
        config.l2Config = {1024, 4, 64, false, 0};   // 1KB, 4-way, 64B blocks
        
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Create accesses that will fill up L1 and cause evictions to L2
        std::vector<std::pair<bool, uint32_t>> accesses;
        
        // Fill up L1 cache
        for (uint32_t i = 0; i < 8; ++i) {
            accesses.push_back({false, 0x1000 + i * 64}); // Read different cache blocks
        }
        
        // Access blocks again - some should be evicted from L1 but found in L2
        for (uint32_t i = 0; i < 8; ++i) {
            accesses.push_back({false, 0x1000 + i * 64}); // Read again
        }
        
        // Apply accesses
        for (const auto& [isWrite, address] : accesses) {
            hierarchy.access(address, isWrite);
        }
        
        // Create a trace file from these accesses
        std::string traceFile = createTestTraceFile(accesses);
        
        // Create a new hierarchy to test the trace
        MemoryHierarchy hierarchy2(config);
        
        // Process the trace file
        hierarchy2.processTrace(traceFile);
        
        // Clean up temporary file
        std::filesystem::remove(traceFile);
        
        // Check that L1 has misses but L2 helps with some hits
        assert(hierarchy2.getL1Misses() > 0 && "Should have some L1 misses");
        assert(hierarchy2.getL1Misses() < accesses.size() && "Should not miss on every access due to L2");
        
        std::cout << "Memory Hierarchy L1-L2 interactions test passed." << std::endl;
    }
    
    // Test write-back policy
    static void testWriteBack() {
        std::cout << "Running Memory Hierarchy write-back test..." << std::endl;
        
        // Create configuration with just L1 for simplicity
        MemoryHierarchyConfig config;
        config.l1Config = {256, 1, 64, false, 0}; // 256 bytes, direct-mapped, 64B blocks
        
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Create accesses to test write-back behavior
        std::vector<std::pair<bool, uint32_t>> accesses = {
            {false, 0x1000},  // Read, should miss
            {true, 0x1000},   // Write, should hit and mark dirty
            {false, 0x2000},  // Read, should miss and potentially cause writeback of 0x1000
            {false, 0x3000},  // Read, should miss and potentially cause writeback of 0x2000
            {true, 0x3000},   // Write, should hit and mark dirty
            {false, 0x1000}   // Read, should miss and cause writeback of 0x3000
        };
        
        // Apply accesses
        for (const auto& [isWrite, address] : accesses) {
            hierarchy.access(address, isWrite);
        }
        
        // Check metrics (exact values depend on cache implementation details)
        assert(hierarchy.getL1Reads() == 4 && "Should have 4 reads");
        assert(hierarchy.getL1Writes() == 2 && "Should have 2 writes");
        
        std::cout << "Memory Hierarchy write-back test passed." << std::endl;
    }
    
    // Test prefetching
    static void testPrefetching() {
        std::cout << "Running Memory Hierarchy prefetching test..." << std::endl;
        
        // Create configuration with prefetching enabled
        MemoryHierarchyConfig config1;
        config1.l1Config = {1024, 2, 64, true, 4}; // With prefetching
        config1.useStridePrediction = true;
        
        // Create configuration without prefetching for comparison
        MemoryHierarchyConfig config2;
        config2.l1Config = {1024, 2, 64, false, 0}; // Without prefetching
        
        // Create memory hierarchies
        MemoryHierarchy hierarchyWithPref(config1);
        MemoryHierarchy hierarchyNoPref(config2);
        
        // Create sequential access pattern (good for prefetching)
        std::vector<std::pair<bool, uint32_t>> accesses;
        for (uint32_t i = 0; i < 32; ++i) {
            accesses.push_back({false, 0x1000 + i * 64}); // Sequential blocks
        }
        
        // Create trace file
        std::string traceFile = createTestTraceFile(accesses);
        
        // Process trace with both hierarchies
        hierarchyWithPref.processTrace(traceFile);
        hierarchyNoPref.processTrace(traceFile);
        
        // Clean up temporary file
        std::filesystem::remove(traceFile);
        
        // With sequential access pattern, prefetching should reduce misses
        assert(hierarchyWithPref.getL1Misses() <= hierarchyNoPref.getL1Misses() && 
              "Prefetching should reduce misses for sequential access");
        
        std::cout << "Memory Hierarchy prefetching test passed." << std::endl;
    }
    
    // Test adaptive prefetching
    static void testAdaptivePrefetching() {
        std::cout << "Running Memory Hierarchy adaptive prefetching test..." << std::endl;
        
        // Create configuration with adaptive prefetching
        MemoryHierarchyConfig config;
        config.l1Config = {1024, 2, 64, true, 4};
        config.useStridePrediction = true;
        config.useAdaptivePrefetching = true;
        
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Create mixed access pattern with some sequential and some random accesses
        std::vector<std::pair<bool, uint32_t>> accesses;
        
        // Sequential pattern
        for (uint32_t i = 0; i < 16; ++i) {
            accesses.push_back({false, 0x1000 + i * 64});
        }
        
        // Different sequential pattern with a stride of 2
        for (uint32_t i = 0; i < 16; ++i) {
            accesses.push_back({false, 0x4000 + i * 128});
        }
        
        // Random accesses
        for (uint32_t i = 0; i < 16; ++i) {
            accesses.push_back({false, 0x8000 + ((i * 1237) % 64) * 64});
        }
        
        // Apply accesses
        for (const auto& [isWrite, address] : accesses) {
            hierarchy.access(address, isWrite);
        }
        
        // Not much to assert here since the behavior is adaptive
        // But we can check that prefetching didn't break anything
        assert(hierarchy.getL1Reads() == accesses.size() && 
              "Should have the correct number of reads");
        
        std::cout << "Memory Hierarchy adaptive prefetching test passed." << std::endl;
    }
    
    // Test comparing two configurations
    static void testComparison() {
        std::cout << "Running Memory Hierarchy comparison test..." << std::endl;
        
        // Create two configurations to compare
        MemoryHierarchyConfig config1;
        config1.l1Config = {512, 1, 64, false, 0}; // Smaller, direct-mapped
        
        MemoryHierarchyConfig config2;
        config2.l1Config = {1024, 2, 64, false, 0}; // Larger, 2-way
        
        // Create memory hierarchies
        MemoryHierarchy hierarchy1(config1);
        MemoryHierarchy hierarchy2(config2);
        
        // Create access pattern with some conflicts
        std::vector<std::pair<bool, uint32_t>> accesses;
        for (uint32_t i = 0; i < 8; ++i) {
            // Access pattern that will cause conflicts in direct-mapped cache
            accesses.push_back({false, 0x1000 + i * 512});
            accesses.push_back({false, 0x1000 + i * 512});
        }
        
        // Create trace file
        std::string traceFile = createTestTraceFile(accesses);
        
        // Process trace with both hierarchies
        hierarchy1.processTrace(traceFile);
        hierarchy2.processTrace(traceFile);
        
        // Clean up temporary file
        std::filesystem::remove(traceFile);
        
        // The larger, more associative cache should have fewer misses
        assert(hierarchy2.getL1Misses() <= hierarchy1.getL1Misses() && 
              "Larger, more associative cache should have fewer misses");
        
        // Test comparison method (just ensure it doesn't crash)
        testing::CaptureStdout();
        hierarchy2.compareWith(hierarchy1);
        std::string output = testing::GetCapturedStdout();
        
        assert(!output.empty() && "Comparison output should not be empty");
        
        std::cout << "Memory Hierarchy comparison test passed." << std::endl;
    }
    
    // Test statistics and metrics
    static void testStatistics() {
        std::cout << "Running Memory Hierarchy statistics test..." << std::endl;
        
        // Create configuration
        MemoryHierarchyConfig config;
        config.l1Config = {512, 2, 64, false, 0};
        
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Create mixed access pattern
        std::vector<std::pair<bool, uint32_t>> accesses = {
            {false, 0x1000}, // Read miss
            {false, 0x1000}, // Read hit
            {true, 0x2000},  // Write miss
            {false, 0x2000}, // Read hit
            {false, 0x3000}, // Read miss
            {true, 0x3000},  // Write hit
            {false, 0x4000}, // Read miss
            {false, 0x1000}  // Read hit
        };
        
        // Apply accesses
        for (const auto& [isWrite, address] : accesses) {
            hierarchy.access(address, isWrite);
        }
        
        // Check statistics
        assert(hierarchy.getL1Reads() == 6 && "Should have 6 reads");
        assert(hierarchy.getL1Writes() == 2 && "Should have 2 writes");
        assert(hierarchy.getL1Misses() == 4 && "Should have 4 misses");
        assert(std::abs(hierarchy.getL1MissRate() - 0.5) < 0.001 && "Miss rate should be 0.5");
        
        // Reset statistics
        hierarchy.resetStats();
        
        // Check reset values
        assert(hierarchy.getL1Reads() == 0 && "Reads should be reset to 0");
        assert(hierarchy.getL1Writes() == 0 && "Writes should be reset to 0");
        assert(hierarchy.getL1Misses() == 0 && "Misses should be reset to 0");
        
        std::cout << "Memory Hierarchy statistics test passed." << std::endl;
    }
    
    // Helper for capturing stdout
    class testing {
    public:
        static void CaptureStdout() {
            // Redirect stdout to our buffer
            oldBuffer = std::cout.rdbuf(buffer.rdbuf());
        }
        
        static std::string GetCapturedStdout() {
            // Restore stdout
            std::cout.rdbuf(oldBuffer);
            return buffer.str();
        }
        
    private:
        static std::stringstream buffer;
        static std::streambuf* oldBuffer;
    };
    
    // Run all tests
    static void runAllTests() {
        std::cout << "Running all Memory Hierarchy tests..." << std::endl;
        
        testInitialization();
        testAccessPatterns();
        testL1L2Interactions();
        testWriteBack();
        testPrefetching();
        testAdaptivePrefetching();
        testComparison();
        testStatistics();
        
        std::cout << "All Memory Hierarchy tests passed!" << std::endl;
    }
};

// Initialize static members of testing
std::stringstream MemoryHierarchyTest::testing::buffer;
std::streambuf* MemoryHierarchyTest::testing::oldBuffer = nullptr;

int main() {
    try {
        MemoryHierarchyTest::runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}