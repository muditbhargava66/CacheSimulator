/**
 * @file victim_cache_performance_test.cpp
 * @brief Performance benchmarks for victim cache implementation
 * @author Mudit Bhargava
 * @date 2025-07-19
 * @version 1.2.0
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <cassert>
#include "../../src/core/victim_cache.h"

using namespace std::chrono;

class VictimCachePerformanceTest {
public:
    static void benchmarkBasicOperations() {
        std::cout << "Benchmarking victim cache basic operations..." << std::endl;
        
        std::vector<size_t> cacheSizes = {4, 8, 16, 32};
        const int numOperations = 100000;
        
        for (size_t size : cacheSizes) {
            VictimCache cache(size);
            
            // Generate random addresses
            std::mt19937 rng(42);
            std::uniform_int_distribution<uint64_t> addrDist(0x1000, 0x100000);
            
            auto start = high_resolution_clock::now();
            
            for (int i = 0; i < numOperations; ++i) {
                uint64_t addr = addrDist(rng) & ~0x3F; // Align to cache line
                
                if (i % 3 == 0) {
                    // Insert operation
                    VictimBlock block(addr, addr >> 6, true, i % 5 == 0);
                    cache.insertBlock(block);
                } else if (i % 3 == 1) {
                    // Find operation
                    cache.findBlock(addr);
                } else {
                    // Search and remove operation
                    cache.searchAndRemove(addr);
                }
            }
            
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start).count();
            
            double opsPerSecond = (numOperations * 1000000.0) / duration;
            
            std::cout << "  Size " << size << ": " << opsPerSecond << " ops/sec" << std::endl;
            std::cout << "    Hit rate: " << (cache.getHitRate() * 100) << "%" << std::endl;
        }
        
        std::cout << "✓ Basic operations benchmark completed!" << std::endl;
    }
    
    static void benchmarkSearchPerformance() {
        std::cout << "Benchmarking victim cache search performance..." << std::endl;
        
        VictimCache cache(64);
        const int numSearches = 50000;
        
        // Pre-populate cache
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint64_t> addrDist(0x1000, 0x10000);
        
        for (int i = 0; i < 32; ++i) {
            uint64_t addr = addrDist(rng) & ~0x3F;
            VictimBlock block(addr, addr >> 6, true, false);
            cache.insertBlock(block);
        }
        
        // Benchmark search operations
        auto start = high_resolution_clock::now();
        
        int hits = 0;
        for (int i = 0; i < numSearches; ++i) {
            uint64_t addr = addrDist(rng) & ~0x3F;
            if (cache.findBlock(addr)) {
                hits++;
            }
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        double searchesPerSecond = (numSearches * 1000000.0) / duration;
        
        std::cout << "  Search rate: " << searchesPerSecond << " searches/sec" << std::endl;
        std::cout << "  Hit rate: " << ((double)hits / numSearches * 100) << "%" << std::endl;
        
        std::cout << "✓ Search performance benchmark completed!" << std::endl;
    }
    
    static void benchmarkEvictionPerformance() {
        std::cout << "Benchmarking victim cache eviction performance..." << std::endl;
        
        VictimCache cache(16);
        const int numInsertions = 10000;
        
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint64_t> addrDist(0x1000, 0x100000);
        
        auto start = high_resolution_clock::now();
        
        for (int i = 0; i < numInsertions; ++i) {
            uint64_t addr = addrDist(rng) & ~0x3F;
            VictimBlock block(addr, addr >> 6, true, i % 3 == 0);
            cache.insertBlock(block);
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        double insertionsPerSecond = (numInsertions * 1000000.0) / duration;
        
        std::cout << "  Insertion rate: " << insertionsPerSecond << " insertions/sec" << std::endl;
        std::cout << "  Final size: " << cache.getCurrentSize() << std::endl;
        std::cout << "  Evictions: " << cache.getEvictions() << std::endl;
        
        std::cout << "✓ Eviction performance benchmark completed!" << std::endl;
    }
    
    static void benchmarkMemoryUsage() {
        std::cout << "Benchmarking victim cache memory usage..." << std::endl;
        
        std::vector<size_t> cacheSizes = {4, 16, 64, 256};
        
        for (size_t size : cacheSizes) {
            VictimCache cache(size);
            
            // Fill cache to capacity
            for (size_t i = 0; i < size; ++i) {
                uint64_t addr = (i + 1) * 0x1000;
                VictimBlock block(addr, addr >> 6, true, false);
                cache.insertBlock(block);
            }
            
            // Estimate memory usage (rough calculation)
            size_t blockSize = sizeof(VictimBlock);
            size_t indexSize = sizeof(std::pair<uint64_t, size_t>);
            size_t queueSize = sizeof(size_t);
            
            size_t estimatedMemory = (blockSize + indexSize + queueSize) * size;
            
            std::cout << "  Size " << size << ": ~" << estimatedMemory << " bytes" << std::endl;
            std::cout << "    Per block: ~" << (estimatedMemory / size) << " bytes" << std::endl;
        }
        
        std::cout << "✓ Memory usage benchmark completed!" << std::endl;
    }
    
    static void runAllBenchmarks() {
        std::cout << "Running Victim Cache Performance Benchmarks..." << std::endl;
        std::cout << "=============================================" << std::endl;
        
        benchmarkBasicOperations();
        std::cout << std::endl;
        
        benchmarkSearchPerformance();
        std::cout << std::endl;
        
        benchmarkEvictionPerformance();
        std::cout << std::endl;
        
        benchmarkMemoryUsage();
        std::cout << std::endl;
        
        std::cout << "All victim cache performance benchmarks completed! ✅" << std::endl;
    }
};

int main() {
    try {
        VictimCachePerformanceTest::runAllBenchmarks();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Performance test failed: " << e.what() << std::endl;
        return 1;
    }
}