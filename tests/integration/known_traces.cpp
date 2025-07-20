#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <optional>
#include <cassert>
#include <cmath>
#include <functional>
#include <chrono>

#include "../../src/core/memory_hierarchy.h"
#include "../../src/utils/trace_parser.h"
#include "../../src/utils/statistics.h"
#include "../../src/utils/logger.h"

using namespace cachesim;
namespace fs = std::filesystem;

// Structure to hold expected metrics for a trace
struct ExpectedMetrics {
    double l1MissRate;
    double l1HitRate;
    int totalAccesses;
    std::optional<double> prefetchAccuracy;
    double tolerance = 0.01; // Tolerance for floating-point comparisons

    // Compare actual metrics to expected with given tolerance
    bool compare(const MemoryHierarchy& hierarchy) const {
        double actualMissRate = hierarchy.getL1MissRate();
        double actualHitRate = 1.0 - actualMissRate;
        int actualTotalAccesses = hierarchy.getTotalMemoryAccesses();
        
        bool result = true;
        
        if (std::abs(actualMissRate - l1MissRate) > tolerance) {
            std::cout << "Miss rate mismatch: expected " << l1MissRate 
                      << ", got " << actualMissRate << std::endl;
            result = false;
        }
        
        if (std::abs(actualHitRate - l1HitRate) > tolerance) {
            std::cout << "Hit rate mismatch: expected " << l1HitRate 
                      << ", got " << actualHitRate << std::endl;
            result = false;
        }
        
        if (actualTotalAccesses != totalAccesses) {
            std::cout << "Total accesses mismatch: expected " << totalAccesses 
                      << ", got " << actualTotalAccesses << std::endl;
            result = false;
        }
        
        if (prefetchAccuracy.has_value()) {
            double actualPrefetchAccuracy = hierarchy.getPrefetchAccuracy();
            if (std::abs(actualPrefetchAccuracy - *prefetchAccuracy) > tolerance) {
                std::cout << "Prefetch accuracy mismatch: expected " << *prefetchAccuracy 
                          << ", got " << actualPrefetchAccuracy << std::endl;
                result = false;
            }
        }
        
        return result;
    }
};

// Trace validation test class
class KnownTracesTest {
public:
    // Test sequential access pattern
    static void testSequentialAccessPattern() {
        std::cout << "Testing sequential access pattern..." << std::endl;
        
        // Create a sequential trace file
        std::string traceFile = createSequentialTrace();
        
        // Expected metrics for different configurations
        std::vector<std::pair<MemoryHierarchyConfig, ExpectedMetrics>> configurations = {
            // Small direct-mapped cache without prefetching
            {createConfig(1024, 1, 64, false, 0),
             {0.9, 0.1, 100, std::nullopt}},
            
            // Larger set-associative cache without prefetching
            {createConfig(4096, 4, 64, false, 0),
             {0.7, 0.3, 100, std::nullopt}},
            
            // Direct-mapped cache with prefetching
            {createConfig(1024, 1, 64, true, 4),
             {0.5, 0.5, 100, 0.8}}
        };
        
        // Test each configuration
        for (const auto& [config, expected] : configurations) {
            // Create memory hierarchy
            MemoryHierarchy hierarchy(config);
            
            // Run the trace
            hierarchy.processTrace(traceFile);
            
            // Verify results
            assert(expected.compare(hierarchy) && "Sequential trace test failed");
        }
        
        // Clean up
        fs::remove(traceFile);
        
        std::cout << "Sequential access pattern test passed!" << std::endl;
    }
    
    // Test random access pattern
    static void testRandomAccessPattern() {
        std::cout << "Testing random access pattern..." << std::endl;
        
        // Create a random trace file
        std::string traceFile = createRandomTrace();
        
        // Expected metrics for different configurations
        std::vector<std::pair<MemoryHierarchyConfig, ExpectedMetrics>> configurations = {
            // Direct-mapped cache
            {createConfig(1024, 1, 64, false, 0),
             {0.95, 0.05, 100, std::nullopt}},
            
            // Set-associative cache
            {createConfig(4096, 4, 64, false, 0),
             {0.85, 0.15, 100, std::nullopt}},
            
            // Cache with prefetching (not very effective for random access)
            {createConfig(4096, 4, 64, true, 4),
             {0.84, 0.16, 100, 0.15}}
        };
        
        // Test each configuration
        for (const auto& [config, expected] : configurations) {
            // Create memory hierarchy
            MemoryHierarchy hierarchy(config);
            
            // Run the trace
            hierarchy.processTrace(traceFile);
            
            // Verify results
            assert(expected.compare(hierarchy) && "Random trace test failed");
        }
        
        // Clean up
        fs::remove(traceFile);
        
        std::cout << "Random access pattern test passed!" << std::endl;
    }
    
    // Test mixed access pattern
    static void testMixedAccessPattern() {
        std::cout << "Testing mixed access pattern..." << std::endl;
        
        // Create a mixed trace file
        std::string traceFile = createMixedTrace();
        
        // Expected metrics for different configurations
        std::vector<std::pair<MemoryHierarchyConfig, ExpectedMetrics>> configurations = {
            // Direct-mapped cache
            {createConfig(2048, 1, 64, false, 0),
             {0.75, 0.25, 200, std::nullopt}},
            
            // Set-associative cache
            {createConfig(8192, 4, 64, false, 0),
             {0.6, 0.4, 200, std::nullopt}},
            
            // Cache with prefetching
            {createConfig(2048, 2, 64, true, 4),
             {0.65, 0.35, 200, 0.4}}
        };
        
        // Test each configuration
        for (const auto& [config, expected] : configurations) {
            // Create memory hierarchy
            MemoryHierarchy hierarchy(config);
            
            // Run the trace
            hierarchy.processTrace(traceFile);
            
            // Verify results
            assert(expected.compare(hierarchy) && "Mixed trace test failed");
        }
        
        // Clean up
        fs::remove(traceFile);
        
        std::cout << "Mixed access pattern test passed!" << std::endl;
    }
    
    // Test loop access pattern
    static void testLoopAccessPattern() {
        std::cout << "Testing loop access pattern..." << std::endl;
        
        // Create a loop trace file
        std::string traceFile = createLoopTrace();
        
        // Expected metrics for different configurations
        std::vector<std::pair<MemoryHierarchyConfig, ExpectedMetrics>> configurations = {
            // Direct-mapped cache (too small for the loop)
            {createConfig(256, 1, 64, false, 0),
             {0.7, 0.3, 200, std::nullopt}},
            
            // Set-associative cache (large enough for the loop)
            {createConfig(1024, 4, 64, false, 0),
             {0.05, 0.95, 200, std::nullopt}},
            
            // Cache with prefetching (helps with the first iteration)
            {createConfig(1024, 4, 64, true, 4),
             {0.03, 0.97, 200, 0.7}}
        };
        
        // Test each configuration
        for (const auto& [config, expected] : configurations) {
            // Create memory hierarchy
            MemoryHierarchy hierarchy(config);
            
            // Run the trace
            hierarchy.processTrace(traceFile);
            
            // Verify results
            assert(expected.compare(hierarchy) && "Loop trace test failed");
        }
        
        // Clean up
        fs::remove(traceFile);
        
        std::cout << "Loop access pattern test passed!" << std::endl;
    }
    
    // Test real trace files if available
    static void testRealTraces() {
        std::cout << "Testing real trace files..." << std::endl;
        
        // Directory containing real trace files
        const std::string traceDir = "../../traces";
        
        // Map of trace files to expected metrics for a standard configuration
        std::unordered_map<std::string, ExpectedMetrics> realTraces = {
            {"trace1.txt", {0.67, 0.33, 9, std::nullopt}},
            {"trace2.txt", {0.5, 0.5, 32, std::nullopt}},
            {"trace_sequential.txt", {0.3, 0.7, 1000, 0.7}}
        };
        
        // Standard configuration for testing
        auto config = createConfig(4096, 4, 64, false, 0);
        
        // Test each trace file if it exists
        for (const auto& [filename, expected] : realTraces) {
            std::string tracePath = traceDir + "/" + filename;
            
            if (fs::exists(tracePath)) {
                std::cout << "  Testing real trace: " << filename << std::endl;
                
                // Create memory hierarchy
                MemoryHierarchy hierarchy(config);
                
                // Run the trace
                hierarchy.processTrace(tracePath);
                
                // Verify results
                assert(expected.compare(hierarchy) && "Real trace test failed");
            } else {
                std::cout << "  Skipping " << filename << " (file not found)" << std::endl;
            }
        }
        
        std::cout << "Real trace tests completed!" << std::endl;
    }
    
    // Test performance benchmarking
    static void testPerformanceBenchmark() {
        std::cout << "Running performance benchmark..." << std::endl;
        
        // Create a large trace file
        std::string traceFile = createLargeTrace(10000);
        
        // Configurations to benchmark
        std::vector<MemoryHierarchyConfig> configs = {
            createConfig(1024, 1, 64, false, 0),  // Baseline
            createConfig(4096, 4, 64, false, 0),  // Larger cache
            createConfig(1024, 1, 64, true, 4)    // With prefetching
        };
        
        std::vector<std::string> names = {
            "Baseline (1KB, direct-mapped)",
            "Larger (4KB, 4-way)",
            "With prefetching"
        };
        
        std::vector<double> missRates;
        std::vector<double> executionTimes;
        
        // Benchmark each configuration
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << "  Testing configuration: " << names[i] << std::endl;
            
            // Create memory hierarchy
            MemoryHierarchy hierarchy(configs[i]);
            
            // Measure execution time
            auto start = std::chrono::high_resolution_clock::now();
            hierarchy.processTrace(traceFile);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            executionTimes.push_back(duration);
            
            // Record miss rate
            missRates.push_back(hierarchy.getL1MissRate());
        }
        
        // Print benchmark results
        std::cout << "Performance Benchmark Results:" << std::endl;
        std::cout << "-----------------------------" << std::endl;
        
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << names[i] << ":" << std::endl;
            std::cout << "  Miss Rate: " << (missRates[i] * 100.0) << "%" << std::endl;
            std::cout << "  Execution Time: " << executionTimes[i] << " ms" << std::endl;
        }
        
        // Clean up
        fs::remove(traceFile);
        
        std::cout << "Performance benchmark completed!" << std::endl;
    }
    
    // Run all tests
    static void runAllTests() {
        std::cout << "Running all known trace tests..." << std::endl;
        
        testSequentialAccessPattern();
        testRandomAccessPattern();
        testMixedAccessPattern();
        testLoopAccessPattern();
        testRealTraces();
        testPerformanceBenchmark();
        
        std::cout << "All known trace tests passed!" << std::endl;
    }

private:
    // Helper to create a configuration
    static MemoryHierarchyConfig createConfig(int l1Size, int l1Assoc, int blockSize, 
                                         bool prefetch, int prefetchDist) {
        MemoryHierarchyConfig config;
        config.l1Config = {l1Size, l1Assoc, blockSize, prefetch, prefetchDist};
        
        // Add L2 if needed
        if (l1Size <= 4096) {
            config.l2Config = CacheConfig{l1Size * 4, l1Assoc * 2, blockSize, prefetch, prefetchDist};
        }
        
        config.useStridePrediction = prefetch;
        config.useAdaptivePrefetching = prefetch;
        
        return config;
    }
    
    // Helper to create a sequential trace file
    static std::string createSequentialTrace() {
        std::string filename = "sequential_trace_" + std::to_string(std::rand()) + ".txt";
        std::ofstream file(filename);
        
        // Generate sequential accesses
        for (int i = 0; i < 100; ++i) {
            file << "r 0x" << std::hex << (0x1000 + i * 64) << std::endl;
        }
        
        file.close();
        return filename;
    }
    
    // Helper to create a random trace file
    static std::string createRandomTrace() {
        std::string filename = "random_trace_" + std::to_string(std::rand()) + ".txt";
        std::ofstream file(filename);
        
        // Seed for reproducibility
        std::srand(42);
        
        // Generate random accesses
        for (int i = 0; i < 100; ++i) {
            uint32_t addr = 0x1000 + (std::rand() % 1000) * 64;
            file << "r 0x" << std::hex << addr << std::endl;
        }
        
        file.close();
        return filename;
    }
    
    // Helper to create a mixed trace file
    static std::string createMixedTrace() {
        std::string filename = "mixed_trace_" + std::to_string(std::rand()) + ".txt";
        std::ofstream file(filename);
        
        // Seed for reproducibility
        std::srand(42);
        
        // Generate sequential accesses
        for (int i = 0; i < 50; ++i) {
            file << "r 0x" << std::hex << (0x1000 + i * 64) << std::endl;
        }
        
        // Generate random accesses
        for (int i = 0; i < 50; ++i) {
            uint32_t addr = 0x10000 + (std::rand() % 1000) * 64;
            file << "r 0x" << std::hex << addr << std::endl;
        }
        
        // Generate mixed writes
        for (int i = 0; i < 50; ++i) {
            if (i % 2 == 0) {
                file << "w 0x" << std::hex << (0x1000 + i * 64) << std::endl;
            } else {
                uint32_t addr = 0x10000 + (std::rand() % 1000) * 64;
                file << "w 0x" << std::hex << addr << std::endl;
            }
        }
        
        // Add repeated accesses
        for (int i = 0; i < 50; ++i) {
            uint32_t addr = 0x1000 + (i % 10) * 64;
            file << "r 0x" << std::hex << addr << std::endl;
        }
        
        file.close();
        return filename;
    }
    
    // Helper to create a loop trace file
    static std::string createLoopTrace() {
        std::string filename = "loop_trace_" + std::to_string(std::rand()) + ".txt";
        std::ofstream file(filename);
        
        // Define a loop access pattern
        std::vector<uint32_t> loopAddresses;
        for (int i = 0; i < 10; ++i) {
            loopAddresses.push_back(0x2000 + i * 64);
        }
        
        // Generate loop accesses (repeat the loop 20 times)
        for (int loop = 0; loop < 20; ++loop) {
            for (uint32_t addr : loopAddresses) {
                file << "r 0x" << std::hex << addr << std::endl;
            }
        }
        
        file.close();
        return filename;
    }
    
    // Helper to create a large trace file for benchmarking
    static std::string createLargeTrace(int size) {
        std::string filename = "large_trace_" + std::to_string(std::rand()) + ".txt";
        std::ofstream file(filename);
        
        // Seed for reproducibility
        std::srand(42);
        
        // Generate a mix of access patterns
        for (int i = 0; i < size; ++i) {
            bool isWrite = (std::rand() % 10) < 3; // 30% writes
            
            uint32_t addr;
            if (i % 3 == 0) {
                // Sequential
                addr = 0x1000 + (i / 3) * 64;
            } else if (i % 3 == 1) {
                // Strided
                addr = 0x10000 + (i / 3) * 128;
            } else {
                // Random
                addr = 0x100000 + (std::rand() % 10000) * 64;
            }
            
            file << (isWrite ? "w " : "r ") << "0x" << std::hex << addr << std::endl;
        }
        
        file.close();
        return filename;
    }
};

int main() {
    try {
        // Initialize logger
        auto& logger = Logger::getInstance();
        logger.setLogLevel(LogLevel::Warning); // Reduce output noise
        
        // Run tests
        KnownTracesTest::runAllTests();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}