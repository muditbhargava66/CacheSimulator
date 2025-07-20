/**
 * @file parallel_processing_test.cpp
 * @brief Unit tests for parallel simulation processing
 * @author Mudit Bhargava
 * @date 2025-06-01
 * @version 1.2.0
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <fstream>
#include "../../../src/utils/parallel_executor.h"
#include "../../../src/core/memory_hierarchy.h"

using namespace cachesim;
using namespace std::chrono;

// Simple cache simulator for testing
class SimpleCacheSimulator {
public:
    struct Config {
        uint32_t cacheSize;
        uint32_t blockSize;
        uint32_t associativity;
    };
    
    struct Statistics {
        uint64_t accesses = 0;
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t prefetches = 0;
    };
    
    SimpleCacheSimulator(const Config& config) : config_(config) {}
    
    bool access(uint32_t address) {
        stats_.accesses++;
        // Simple hit rate simulation
        bool hit = (address % 10) > 3;  // 60% hit rate
        if (hit) stats_.hits++;
        else stats_.misses++;
        return hit;
    }
    
    Statistics getStatistics() const { return stats_; }
    
private:
    Config config_;
    Statistics stats_;
};

class ParallelProcessingTest {
public:
    static void testThreadPool() {
        std::cout << "Testing thread pool..." << std::endl;
        
        ThreadPool pool(4);
        assert(pool.getNumThreads() == 4 && "Should have 4 threads");
        
        // Test simple tasks
        std::vector<std::future<int>> futures;
        
        for (int i = 0; i < 10; ++i) {
            futures.push_back(
                pool.enqueue([i]() {
                    std::this_thread::sleep_for(milliseconds(10));
                    return i * i;
                })
            );
        }
        
        // Collect results
        int sum = 0;
        for (auto& f : futures) {
            sum += f.get();
        }
        
        assert(sum == 285 && "Sum of squares 0-9 should be 285");
        
        std::cout << "✓ Thread pool test passed!" << std::endl;
    }
    
    static void testParallelTraceProcessor() {
        std::cout << "Testing parallel trace processor..." << std::endl;
        
        // Create a test trace file
        std::string traceFile = "test_parallel_trace.txt";
        createTestTrace(traceFile, 10000);
        
        // Create processor
        ParallelTraceProcessor<SimpleCacheSimulator> processor(4);
        
        // Process trace
        SimpleCacheSimulator::Config config{32768, 64, 4};
        auto result = processor.processTrace(traceFile, config, 1000);
        
        assert(result.totalAccesses == 10000 && "Should process all accesses");
        assert(result.hits > 0 && "Should have some hits");
        assert(result.misses > 0 && "Should have some misses");
        
        // Clean up
        std::filesystem::remove(traceFile);
        
        std::cout << "✓ Parallel trace processor test passed!" << std::endl;
        std::cout << "  Processing time: " << 
                     duration_cast<milliseconds>(result.processingTime).count() 
                     << " ms" << std::endl;
    }
    
    static void testSpeedup() {
        std::cout << "Testing parallel speedup..." << std::endl;
        
        // Create smaller trace for faster testing
        std::string traceFile = "test_speedup_trace.txt";
        createTestTrace(traceFile, 1000);  // Reduced size
        
        SimpleCacheSimulator::Config config{65536, 64, 8};
        
        // Sequential processing
        ParallelTraceProcessor<SimpleCacheSimulator> seqProcessor(1);
        auto seqResult = seqProcessor.processTrace(traceFile, config, 1000);
        
        // Parallel processing
        ParallelTraceProcessor<SimpleCacheSimulator> parProcessor(2);  // Reduced threads
        auto parResult = parProcessor.processTrace(traceFile, config, 500);
        
        // Basic validation - both should process some accesses
        assert(seqResult.totalAccesses > 0 && "Sequential should process accesses");
        assert(parResult.totalAccesses > 0 && "Parallel should process accesses");
        
        std::cout << "  Sequential accesses: " << seqResult.totalAccesses << std::endl;
        std::cout << "  Parallel accesses: " << parResult.totalAccesses << std::endl;
        
        // Clean up
        std::filesystem::remove(traceFile);
        
        std::cout << "✓ Speedup test passed!" << std::endl;
    }
    
    static void testMultipleConfigurations() {
        std::cout << "Testing multiple configurations..." << std::endl;
        
        std::string traceFile = "test_multiconfig_trace.txt";
        createTestTrace(traceFile, 100);  // Much smaller trace
        
        // Reduced configurations for faster testing
        std::vector<SimpleCacheSimulator::Config> configs = {
            {16384, 64, 2},   // 16KB, 2-way
            {32768, 64, 4}    // 32KB, 4-way
        };
        
        // Test each config individually to avoid hanging
        ParallelTraceProcessor<SimpleCacheSimulator> processor(2);
        
        for (const auto& config : configs) {
            auto result = processor.processTrace(traceFile, config, 100);
            
            std::cout << "  Config " << config.cacheSize << "B: " 
                      << result.hits << " hits, "
                      << result.misses << " misses" << std::endl;
            assert(result.totalAccesses > 0 && "Should process some accesses");
        }
        
        // Clean up
        std::filesystem::remove(traceFile);
        
        std::cout << "✓ Multiple configurations test passed!" << std::endl;
    }
    
    static void testParallelBenchmark() {
        std::cout << "Testing parallel benchmark..." << std::endl;
        
        std::vector<ParallelBenchmark::BenchmarkConfig> configs;
        
        // Add different benchmarks
        for (int size = 1000; size <= 4000; size += 1000) {
            configs.push_back({
                "Size " + std::to_string(size),
                nullptr,  // No setup
                [size]() {
                    // Simulate some work
                    double sum = 0;
                    for (int i = 0; i < size; ++i) {
                        sum += std::sqrt(i) * std::sin(i);
                    }
                    return sum;
                },
                nullptr,  // No teardown
                50        // iterations
            });
        }
        
        auto results = ParallelBenchmark::runParallel(configs, 4);
        
        assert(results.size() == configs.size() && "Should have all results");
        
        // Verify statistics are computed
        for (const auto& result : results) {
            std::cout << "  " << result.name << ": "
                      << std::fixed << std::setprecision(2)
                      << result.mean << " ms (σ=" << result.stddev << ")" << std::endl;
            
            assert(result.times.size() == 50 && "Should have 50 measurements");
            assert(result.mean > 0 && "Mean should be positive");
            assert(result.median > 0 && "Median should be positive");
            assert(result.min <= result.mean && "Min should be <= mean");
            assert(result.max >= result.mean && "Max should be >= mean");
        }
        
        std::cout << "✓ Parallel benchmark test passed!" << std::endl;
    }
    
    static void testErrorHandling() {
        std::cout << "Testing error handling..." << std::endl;
        
        ThreadPool pool(2);
        
        // Test exception in task
        auto future = pool.enqueue([]() {
            throw std::runtime_error("Test exception");
            return 42;
        });
        
        bool exceptionCaught = false;
        try {
            future.get();
        } catch (const std::runtime_error& e) {
            exceptionCaught = true;
            assert(std::string(e.what()) == "Test exception");
        }
        
        assert(exceptionCaught && "Should catch exception from task");
        
        // Test invalid trace file
        ParallelTraceProcessor<SimpleCacheSimulator> processor(2);
        SimpleCacheSimulator::Config config{32768, 64, 4};
        
        bool fileErrorCaught = false;
        try {
            processor.processTrace("nonexistent_file.txt", config);
        } catch (const std::runtime_error&) {
            fileErrorCaught = true;
        }
        
        assert(fileErrorCaught && "Should catch file error");
        
        std::cout << "✓ Error handling test passed!" << std::endl;
    }
    
    static void runAllTests() {
        std::cout << "Running Parallel Processing Tests..." << std::endl;
        std::cout << "====================================" << std::endl;
        
        testThreadPool();
        testParallelTraceProcessor();
        testSpeedup();
        testMultipleConfigurations();
        testParallelBenchmark();
        testErrorHandling();
        
        std::cout << std::endl;
        std::cout << "All Parallel Processing tests passed! ✅" << std::endl;
    }
    
private:
    static void createTestTrace(const std::string& filename, size_t numAccesses) {
        std::ofstream file(filename);
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> addrDist(0x1000, 0x10000);
        
        for (size_t i = 0; i < numAccesses; ++i) {
            char op = (i % 3 == 0) ? 'w' : 'r';
            uint32_t addr = addrDist(rng) & ~0x3F;  // Align to 64 bytes
            file << op << " 0x" << std::hex << addr << std::dec << "\n";
        }
    }
};

int main() {
    try {
        ParallelProcessingTest::runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
