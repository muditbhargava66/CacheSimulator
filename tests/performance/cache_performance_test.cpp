/**
 * @file cache_performance_test.cpp
 * @brief Performance benchmarks for cache simulation
 * @author Mudit Bhargava
 * @date 2025-07-19
 * @version 1.2.0
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <fstream>
#include <cassert>
#include "../../src/core/memory_hierarchy.h"
#include "../../src/core/cache.h"
#include "../../src/utils/trace_parser.h"

using namespace cachesim;
using namespace std::chrono;

class CachePerformanceTest {
public:
    static void benchmarkBasicCacheOperations() {
        std::cout << "Benchmarking basic cache operations..." << std::endl;
        
        // Test different cache configurations
        std::vector<CacheConfig> configs = {
            {16384, 2, 64},   // 16KB, 2-way
            {32768, 4, 64},   // 32KB, 4-way
            {65536, 8, 64},   // 64KB, 8-way
        };
        
        const int numAccesses = 100000;
        
        for (const auto& config : configs) {
            Cache cache(config);
            
            // Generate random addresses
            std::mt19937 rng(42);
            std::uniform_int_distribution<uint32_t> addrDist(0x1000, 0x100000);
            
            auto start = high_resolution_clock::now();
            
            for (int i = 0; i < numAccesses; ++i) {
                uint32_t addr = addrDist(rng) & ~0x3F; // Align to cache line
                cache.access(addr, i % 4 == 0); // 25% writes
            }
            
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start).count();
            
            double accessesPerSecond = (numAccesses * 1000000.0) / duration;
            
            std::cout << "  " << config.size/1024 << "KB " << config.associativity 
                      << "-way: " << accessesPerSecond << " accesses/sec" << std::endl;
        }
        
        std::cout << "✓ Basic cache operations benchmark completed!" << std::endl;
    }
    
    static void benchmarkReplacementPolicies() {
        std::cout << "Benchmarking replacement policies..." << std::endl;
        
        std::vector<ReplacementPolicy> policies = {
            ReplacementPolicy::LRU,
            ReplacementPolicy::FIFO,
            ReplacementPolicy::Random,
            ReplacementPolicy::NRU
        };
        
        const int numAccesses = 50000;
        CacheConfig baseConfig{32768, 4, 64};
        
        for (auto policy : policies) {
            CacheConfig config = baseConfig;
            config.replacementPolicy = policy;
            
            Cache cache(config);
            
            // Generate addresses with some locality
            std::mt19937 rng(42);
            std::normal_distribution<double> addrDist(0x10000, 1000);
            
            auto start = high_resolution_clock::now();
            
            for (int i = 0; i < numAccesses; ++i) {
                uint32_t addr = static_cast<uint32_t>(std::abs(addrDist(rng))) & ~0x3F;
                cache.access(addr, false);
            }
            
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start).count();
            
            double accessesPerSecond = (numAccesses * 1000000.0) / duration;
            
            std::cout << "  " << replacementPolicyToString(policy) 
                      << ": " << accessesPerSecond << " accesses/sec" << std::endl;
        }
        
        std::cout << "✓ Replacement policies benchmark completed!" << std::endl;
    }
    
    static void benchmarkMemoryHierarchy() {
        std::cout << "Benchmarking memory hierarchy..." << std::endl;
        
        MemoryHierarchyConfig config;
        config.l1Config = {32768, 4, 64};
        config.l2Config = CacheConfig{262144, 8, 64};
        
        MemoryHierarchy hierarchy(config);
        
        const int numAccesses = 100000;
        
        // Generate mixed access pattern
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> addrDist(0x1000, 0x200000);
        
        auto start = high_resolution_clock::now();
        
        for (int i = 0; i < numAccesses; ++i) {
            uint32_t addr = addrDist(rng) & ~0x3F;
            hierarchy.access(addr, i % 5 == 0); // 20% writes
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        double accessesPerSecond = (numAccesses * 1000000.0) / duration;
        
        std::cout << "  Memory hierarchy: " << accessesPerSecond << " accesses/sec" << std::endl;
        std::cout << "  L1 hit rate: " << (hierarchy.getL1HitRate() * 100) << "%" << std::endl;
        
        std::cout << "✓ Memory hierarchy benchmark completed!" << std::endl;
    }
    
    static void benchmarkTraceProcessing() {
        std::cout << "Benchmarking trace processing..." << std::endl;
        
        // Create test trace
        std::string traceFile = "perf_test_trace.txt";
        createLargeTrace(traceFile, 50000);
        
        MemoryHierarchyConfig config;
        config.l1Config = {32768, 4, 64};
        config.l2Config = CacheConfig{262144, 8, 64};
        
        MemoryHierarchy hierarchy(config);
        
        auto start = high_resolution_clock::now();
        
        TraceParser parser(traceFile);
        int accessCount = 0;
        
        while (auto access = parser.getNextAccess()) {
            hierarchy.access(access->address, access->isWrite);
            accessCount++;
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        
        double accessesPerSecond = (accessCount * 1000.0) / duration;
        
        std::cout << "  Trace processing: " << accessesPerSecond << " accesses/sec" << std::endl;
        std::cout << "  Total accesses: " << accessCount << std::endl;
        std::cout << "  Processing time: " << duration << " ms" << std::endl;
        
        // Clean up
        std::filesystem::remove(traceFile);
        
        std::cout << "✓ Trace processing benchmark completed!" << std::endl;
    }
    
    static void runAllBenchmarks() {
        std::cout << "Running Cache Performance Benchmarks..." << std::endl;
        std::cout << "=======================================" << std::endl;
        
        benchmarkBasicCacheOperations();
        std::cout << std::endl;
        
        benchmarkReplacementPolicies();
        std::cout << std::endl;
        
        benchmarkMemoryHierarchy();
        std::cout << std::endl;
        
        benchmarkTraceProcessing();
        std::cout << std::endl;
        
        std::cout << "All performance benchmarks completed! ✅" << std::endl;
    }
    
private:
    static void createLargeTrace(const std::string& filename, int numAccesses) {
        std::ofstream file(filename);
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> addrDist(0x1000, 0x100000);
        
        for (int i = 0; i < numAccesses; ++i) {
            char op = (i % 4 == 0) ? 'W' : 'R';
            uint32_t addr = addrDist(rng) & ~0x3F;
            file << op << " 0x" << std::hex << addr << std::dec << "\n";
        }
    }
};

int main() {
    try {
        CachePerformanceTest::runAllBenchmarks();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Performance test failed: " << e.what() << std::endl;
        return 1;
    }
}