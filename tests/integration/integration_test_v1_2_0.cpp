/**
 * @file integration_test_v1_2_0.cpp
 * @brief Integration tests for all v1.2.0 features
 * @author Mudit Bhargava
 * @date 2025-06-01
 * @version 1.2.0
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include <fstream>
#include <random>
#include "../../src/core/memory_hierarchy.h"
#include "../../src/core/victim_cache.h"
#include "../../src/core/multiprocessor/processor_core.h"
#include "../../src/utils/parallel_executor.h"
#include "../../src/utils/visualization.h"
#include "../../src/utils/config_utils.h"
#include "../../src/utils/trace_parser.h"
#include "../../src/core/write_policy.h"

using namespace cachesim;

class IntegrationTestV120 {
public:
    static void testCompleteSystemWithAllFeatures() {
        std::cout << "Testing complete system with all v1.2.0 features..." << std::endl;
        
        // Configure system with all new features
        MemoryHierarchyConfig config;
        
        // L1 with NRU policy and victim cache
        config.l1Config = CacheConfig(32768, 4, 64, true, 4);
        config.l1Config.replacementPolicy = ReplacementPolicy::NRU;
        config.l1Config.writePolicy = WritePolicy::WriteBack;
        
        // L2 with write-through no-allocate
        config.l2Config = CacheConfig(262144, 8, 64, true, 8);
        config.l2Config->replacementPolicy = ReplacementPolicy::LRU;
        config.l2Config->writePolicy = WritePolicy::WriteThrough;
        
        // Enable advanced features
        config.useStridePrediction = true;
        config.useAdaptivePrefetching = true;
        
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Create victim cache
        VictimCache victimCache(8);
        
        // Generate test workload
        std::vector<uint32_t> addresses = generateTestWorkload();
        
        // Process workload
        for (uint32_t addr : addresses) {
            bool isWrite = (addr % 3) == 0;
            hierarchy.access(addr, isWrite);
            
            // Simulate victim cache interaction
            if (!hierarchy.getL1Cache()) {
                // On L1 miss, check victim cache
                if (victimCache.findBlock(addr)) {
                    // Hit in victim cache
                    auto block = victimCache.searchAndRemove(addr);
                    // Would reinstall in L1
                }
            }
        }
        
        // Verify statistics
        hierarchy.printStats();
        
        double l1MissRate = hierarchy.getL1MissRate();
        assert(l1MissRate >= 0.0 && l1MissRate <= 1.0 && 
               "L1 miss rate should be valid");
        
        std::cout << "  Victim cache hit rate: " << 
                     (victimCache.getHitRate() * 100) << "%" << std::endl;
        
        std::cout << "✓ Complete system test passed!" << std::endl;
    }
    
    static void testParallelMultiProcessorSimulation() {
        std::cout << "Testing parallel multi-processor simulation..." << std::endl;
        
        // Configure 4-processor system
        MultiProcessorSystem::Config mpConfig;
        mpConfig.numProcessors = 4;
        mpConfig.l1Config = CacheConfig(16384, 2, 64);
        mpConfig.l1Config.replacementPolicy = ReplacementPolicy::NRU;
        mpConfig.enableCoherence = true;
        mpConfig.interconnectLatency = 10;
        
        MultiProcessorSystem system(mpConfig);
        
        // Create per-processor traces
        std::vector<std::string> traceFiles;
        for (int i = 0; i < 4; ++i) {
            std::string filename = "mp_trace_" + std::to_string(i) + ".txt";
            createProcessorTrace(filename, i, 1000);
            traceFiles.push_back(filename);
        }
        
        // Run parallel simulation
        auto startTime = std::chrono::high_resolution_clock::now();
        uint64_t cycles = system.simulateParallelTraces(traceFiles);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();
        
        std::cout << "  Simulation completed in " << duration << " ms" << std::endl;
        std::cout << "  Total cycles: " << cycles << std::endl;
        
        // Print system statistics
        system.printSystemStats();
        
        // Clean up trace files
        for (const auto& file : traceFiles) {
            std::filesystem::remove(file);
        }
        
        std::cout << "✓ Parallel multi-processor test passed!" << std::endl;
    }
    
    static void testVisualizationWithRealData() {
        std::cout << "Testing visualization with real simulation data..." << std::endl;
        
        // Run simulation to collect data
        CacheConfig config(32768, 4, 64);
        config.replacementPolicy = ReplacementPolicy::NRU;
        Cache cache(config);
        
        // Track hit rate over time
        std::vector<std::pair<double, double>> hitRateHistory;
        std::vector<std::pair<std::string, double>> missTypeStats;
        
        const int windowSize = 100;
        int windowHits = 0;
        int windowAccesses = 0;
        
        // Generate accesses
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> addrDist(0x1000, 0x10000);
        
        for (int i = 0; i < 10000; ++i) {
            uint32_t addr = addrDist(rng) & ~0x3F;
            bool hit = cache.access(addr, i % 3 == 0);
            
            windowAccesses++;
            if (hit) windowHits++;
            
            // Record hit rate every window
            if (windowAccesses >= windowSize) {
                double hitRate = (double)windowHits / windowAccesses * 100;
                hitRateHistory.push_back({i / 1000.0, hitRate});
                windowHits = windowAccesses = 0;
            }
        }
        
        // Get miss type statistics
        auto missTypes = cache.getMissTypeStats();
        missTypeStats = {
            {"Compulsory", (double)missTypes[0]},
            {"Capacity", (double)missTypes[1]},
            {"Conflict", (double)missTypes[2]},
            {"Coherence", (double)missTypes[3]}
        };
        
        // Generate visualizations
        std::cout << "\nHit Rate Over Time:" << std::endl;
        std::cout << Visualization::generateLineChart(
            hitRateHistory, 60, 15,
            "Cache Hit Rate Evolution",
            "Time (K accesses)", "Hit Rate (%)"
        ) << std::endl;
        
        std::cout << "\nMiss Type Distribution:" << std::endl;
        std::cout << Visualization::generatePieChart(missTypeStats, 12) << std::endl;
        
        std::cout << "\nCache Performance Metrics:" << std::endl;
        std::vector<std::pair<std::string, double>> metrics = {
            {"Hit Rate", cache.getHitRatio() * 100},
            {"Miss Rate", cache.getMissRatio() * 100},
            {"Efficiency", cache.getCacheEfficiency() * 100}
        };
        std::cout << Visualization::generateHistogram(metrics, 40) << std::endl;
        
        std::cout << "✓ Visualization test passed!" << std::endl;
    }
    
    static void testWritePolicyInteractions() {
        std::cout << "Testing write policy interactions..." << std::endl;
        
        // Create caches with different write policies
        CacheConfig wtConfig(8192, 2, 64);
        wtConfig.writePolicy = WritePolicy::WriteThrough;
        
        CacheConfig wbConfig(8192, 2, 64);
        wbConfig.writePolicy = WritePolicy::WriteBack;
        
        Cache writeThrough(wtConfig);
        Cache writeBack(wbConfig);
        
        // Test with write combining buffer
        WriteCombiningBuffer combiner(8);
        
        // Simulate write-heavy workload
        std::vector<uint32_t> writeAddresses;
        for (uint32_t addr = 0x1000; addr < 0x2000; addr += 64) {
            writeAddresses.push_back(addr);
            writeAddresses.push_back(addr);  // Repeated writes
        }
        
        // Process writes
        for (uint32_t addr : writeAddresses) {
            // Try combining first
            if (!combiner.tryWrite(addr)) {
                // Buffer full, flush and process
                auto toFlush = combiner.flush();
                for (uint32_t flushAddr : toFlush) {
                    writeThrough.access(flushAddr, true);
                    writeBack.access(flushAddr, true);
                }
                combiner.tryWrite(addr);
            }
        }
        
        // Final flush
        auto remaining = combiner.flush();
        for (uint32_t addr : remaining) {
            writeThrough.access(addr, true);
            writeBack.access(addr, true);
        }
        
        // Compare statistics
        std::cout << "  Write-through writes: " << writeThrough.getWrites() << std::endl;
        std::cout << "  Write-back writes: " << writeBack.getWrites() << std::endl;
        std::cout << "  Write-back writebacks: " << writeBack.getWriteBacks() << std::endl;
        
        assert(writeBack.getWriteBacks() < writeThrough.getWrites() &&
               "Write-back should have fewer memory writes");
        
        std::cout << "✓ Write policy interactions test passed!" << std::endl;
    }
    
    static void testEndToEndPerformance() {
        std::cout << "Testing end-to-end performance improvements..." << std::endl;
        
        // Baseline configuration
        MemoryHierarchyConfig baselineConfig;
        baselineConfig.l1Config = CacheConfig(32768, 4, 64);
        baselineConfig.l1Config.replacementPolicy = ReplacementPolicy::LRU;
        
        // v1.2.0 optimized configuration
        MemoryHierarchyConfig optimizedConfig;
        optimizedConfig.l1Config = CacheConfig(32768, 4, 64, true, 4);
        optimizedConfig.l1Config.replacementPolicy = ReplacementPolicy::NRU;
        optimizedConfig.l2Config = CacheConfig(262144, 8, 64, true, 8);
        optimizedConfig.useAdaptivePrefetching = true;
        
        // Create workload
        std::vector<MemoryAccess> workload = generateRealisticWorkload(10000);
        
        // Measure baseline performance
        auto baselineStart = std::chrono::high_resolution_clock::now();
        MemoryHierarchy baseline(baselineConfig);
        for (const auto& access : workload) {
            baseline.access(access.address, access.isWrite);
        }
        auto baselineEnd = std::chrono::high_resolution_clock::now();
        
        // Measure optimized performance
        auto optimizedStart = std::chrono::high_resolution_clock::now();
        MemoryHierarchy optimized(optimizedConfig);
        VictimCache victimCache(8);
        
        for (const auto& access : workload) {
            optimized.access(access.address, access.isWrite);
        }
        auto optimizedEnd = std::chrono::high_resolution_clock::now();
        
        // Calculate improvements
        auto baselineTime = std::chrono::duration_cast<std::chrono::microseconds>(
            baselineEnd - baselineStart).count();
        auto optimizedTime = std::chrono::duration_cast<std::chrono::microseconds>(
            optimizedEnd - optimizedStart).count();
        
        double speedup = (double)baselineTime / optimizedTime;
        double missReduction = (baseline.getL1MissRate() - optimized.getL1MissRate()) 
                              / baseline.getL1MissRate() * 100;
        
        std::cout << "  Baseline time: " << baselineTime << " μs" << std::endl;
        std::cout << "  Optimized time: " << optimizedTime << " μs" << std::endl;
        std::cout << "  Speedup: " << speedup << "x" << std::endl;
        std::cout << "  Miss rate reduction: " << missReduction << "%" << std::endl;
        
        assert(optimized.getL1MissRate() <= baseline.getL1MissRate() &&
               "Optimized config should have better or equal miss rate");
        
        std::cout << "✓ End-to-end performance test passed!" << std::endl;
    }
    
    static void runAllTests() {
        std::cout << "Running v1.2.0 Integration Tests..." << std::endl;
        std::cout << "===================================" << std::endl;
        
        testCompleteSystemWithAllFeatures();
        testParallelMultiProcessorSimulation();
        testVisualizationWithRealData();
        testWritePolicyInteractions();
        testEndToEndPerformance();
        
        std::cout << std::endl;
        std::cout << "All v1.2.0 Integration tests passed! ✅" << std::endl;
    }
    
private:
    static std::vector<uint32_t> generateTestWorkload() {
        std::vector<uint32_t> addresses;
        std::mt19937 rng(42);
        
        // Mix of access patterns
        // Sequential
        for (uint32_t addr = 0x1000; addr < 0x2000; addr += 64) {
            addresses.push_back(addr);
        }
        
        // Strided
        for (uint32_t addr = 0x4000; addr < 0x8000; addr += 256) {
            addresses.push_back(addr);
        }
        
        // Random with locality
        std::normal_distribution<double> localDist(0, 200);
        uint32_t base = 0x10000;
        for (int i = 0; i < 1000; ++i) {
            int offset = std::abs((int)localDist(rng));
            addresses.push_back(base + (offset & ~0x3F));
            if (i % 100 == 0) base += 0x1000;
        }
        
        return addresses;
    }
    
    static void createProcessorTrace(const std::string& filename, 
                                   int processorId, int numAccesses) {
        std::ofstream file(filename);
        std::mt19937 rng(processorId);
        
        // Each processor accesses different memory regions with some overlap
        uint32_t baseAddr = 0x10000 + processorId * 0x4000;
        uint32_t sharedAddr = 0x20000;  // Shared region
        
        for (int i = 0; i < numAccesses; ++i) {
            uint32_t addr;
            char op = (i % 3 == 0) ? 'w' : 'r';
            
            if (i % 10 < 7) {
                // Private access
                addr = baseAddr + (rng() % 0x4000) & ~0x3F;
            } else {
                // Shared access
                addr = sharedAddr + (rng() % 0x1000) & ~0x3F;
            }
            
            file << op << " 0x" << std::hex << addr << std::dec << "\n";
        }
    }
    
    static std::vector<MemoryAccess> generateRealisticWorkload(int size) {
        std::vector<MemoryAccess> workload;
        std::mt19937 rng(12345);
        
        // Matrix multiplication pattern
        const int matrixSize = 64;
        for (int i = 0; i < matrixSize && workload.size() < size; ++i) {
            for (int j = 0; j < matrixSize && workload.size() < size; ++j) {
                for (int k = 0; k < matrixSize && workload.size() < size; ++k) {
                    // A[i][k]
                    workload.push_back({false, static_cast<uint32_t>(0x10000 + (i * matrixSize + k) * 8)});
                    // B[k][j]
                    workload.push_back({false, static_cast<uint32_t>(0x20000 + (k * matrixSize + j) * 8)});
                    // C[i][j]
                    if (k == 0) {
                        workload.push_back({false, static_cast<uint32_t>(0x30000 + (i * matrixSize + j) * 8)});
                    }
                    workload.push_back({true, static_cast<uint32_t>(0x30000 + (i * matrixSize + j) * 8)});
                }
            }
        }
        
        return workload;
    }
};

int main() {
    try {
        IntegrationTestV120::runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
