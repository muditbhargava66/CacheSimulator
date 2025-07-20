/**
 * @file multiprocessor_performance_test.cpp
 * @brief Performance benchmarks for multi-processor simulation
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
#include "../../src/core/multiprocessor/interconnect.h"
#include "../../src/core/cache.h"
#include "../../src/core/mesi_protocol.h"

using namespace cachesim;
using namespace std::chrono;

class MultiprocessorPerformanceTest {
public:
    static void benchmarkInterconnectTypes() {
        std::cout << "Benchmarking interconnect types..." << std::endl;
        
        std::vector<InterconnectType> types = {
            InterconnectType::Bus,
            InterconnectType::Crossbar,
            InterconnectType::Mesh
        };
        
        const int numMessages = 1000;  // Reduced for simpler testing
        const int numProcessors = 4;
        
        for (auto type : types) {
            auto interconnect = InterconnectFactory::create(type, numProcessors, 10);
            
            std::mt19937 rng(42);
            std::uniform_int_distribution<uint32_t> procDist(0, numProcessors - 1);
            
            auto start = high_resolution_clock::now();
            
            for (int i = 0; i < numMessages; ++i) {
                InterconnectMessage msg;
                msg.sourceId = procDist(rng);
                msg.destId = procDist(rng);
                msg.address = 0x1000 + (i * 64);
                msg.payload.resize(64);
                
                interconnect->sendMessage(msg);
            }
            
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start).count();
            
            double messagesPerSecond = (numMessages * 1000000.0) / duration;
            
            std::string typeName;
            switch (type) {
                case InterconnectType::Bus: typeName = "Bus"; break;
                case InterconnectType::Crossbar: typeName = "Crossbar"; break;
                case InterconnectType::Mesh: typeName = "Mesh"; break;
                default: typeName = "Unknown"; break;
            }
            
            std::cout << "  " << typeName << ": " << messagesPerSecond << " messages/sec" << std::endl;
            
            auto stats = interconnect->getStats();
            std::cout << "    Total messages: " << stats.totalMessages << std::endl;
        }
        
        std::cout << "✓ Interconnect types benchmark completed!" << std::endl;
    }
    
    static void benchmarkCacheCoherence() {
        std::cout << "Benchmarking cache coherence simulation..." << std::endl;
        
        const int numCaches = 4;
        const int numAccesses = 5000;
        
        // Create multiple caches to simulate coherence
        std::vector<std::unique_ptr<Cache>> caches;
        for (int i = 0; i < numCaches; ++i) {
            CacheConfig config{16384, 2, 64};  // Smaller for faster testing
            caches.push_back(std::make_unique<Cache>(config));
        }
        
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> cacheDist(0, numCaches - 1);
        std::uniform_int_distribution<uint64_t> addrDist(0x1000, 0x10000);
        
        auto start = high_resolution_clock::now();
        
        for (int i = 0; i < numAccesses; ++i) {
            int cacheId = cacheDist(rng);
            uint64_t addr = addrDist(rng) & ~0x3F;
            bool isWrite = (i % 5 == 0);
            
            // Simulate cache access
            caches[cacheId]->access(addr, isWrite);
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        double accessesPerSecond = (numAccesses * 1000000.0) / duration;
        
        std::cout << "  Cache accesses: " << accessesPerSecond << " accesses/sec" << std::endl;
        std::cout << "  Number of caches: " << numCaches << std::endl;
        
        std::cout << "✓ Cache coherence benchmark completed!" << std::endl;
    }
    
    static void benchmarkMESIProtocol() {
        std::cout << "Benchmarking MESI protocol..." << std::endl;
        
        const int numOperations = 10000;
        MESIProtocol protocol;
        
        auto start = high_resolution_clock::now();
        
        for (int i = 0; i < numOperations; ++i) {
            bool isWrite = (i % 4 == 0);
            bool otherCachesHaveCopy = (i % 3 == 0);
            
            // Simulate MESI state transitions
            MESIState currentState = static_cast<MESIState>(i % 4);
            MESIState newState;
            
            if (isWrite) {
                newState = protocol.handleLocalWrite(currentState);
            } else {
                newState = protocol.handleLocalRead(currentState, otherCachesHaveCopy);
            }
            
            // Basic validation
            assert(newState >= MESIState::Invalid && newState <= MESIState::Modified);
        }
        
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();
        
        double operationsPerSecond = (numOperations * 1000000.0) / duration;
        
        std::cout << "  MESI operations: " << operationsPerSecond << " ops/sec" << std::endl;
        
        std::cout << "✓ MESI protocol benchmark completed!" << std::endl;
    }
    
    static void benchmarkScalability() {
        std::cout << "Benchmarking scalability..." << std::endl;
        
        std::vector<int> cacheCounts = {2, 4, 8};
        const int accessesPerCache = 1000;
        
        for (int numCaches : cacheCounts) {
            auto start = high_resolution_clock::now();
            
            // Create multiple caches to simulate multi-processor system
            std::vector<std::unique_ptr<Cache>> caches;
            for (int i = 0; i < numCaches; ++i) {
                CacheConfig l1Config{16384, 2, 64};  // Smaller cache for faster testing
                caches.push_back(std::make_unique<Cache>(l1Config));
            }
            
            // Simulate accesses
            std::mt19937 rng(42);
            std::uniform_int_distribution<uint64_t> addrDist(0x1000, 0x10000);
            
            for (int cache = 0; cache < numCaches; ++cache) {
                for (int i = 0; i < accessesPerCache; ++i) {
                    uint64_t addr = addrDist(rng) & ~0x3F;
                    caches[cache]->access(addr, i % 4 == 0);
                }
            }
            
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start).count();
            
            int totalAccesses = numCaches * accessesPerCache;
            double accessesPerSecond = (totalAccesses * 1000.0) / duration;
            
            std::cout << "  " << numCaches << " caches: " << accessesPerSecond << " accesses/sec" << std::endl;
            std::cout << "    Total accesses: " << totalAccesses << std::endl;
            std::cout << "    Time: " << duration << " ms" << std::endl;
        }
        
        std::cout << "✓ Scalability benchmark completed!" << std::endl;
    }
    
    static void runAllBenchmarks() {
        std::cout << "Running Multi-processor Performance Benchmarks..." << std::endl;
        std::cout << "================================================" << std::endl;
        
        benchmarkInterconnectTypes();
        std::cout << std::endl;
        
        benchmarkCacheCoherence();
        std::cout << std::endl;
        
        benchmarkMESIProtocol();
        std::cout << std::endl;
        
        benchmarkScalability();
        std::cout << std::endl;
        
        std::cout << "All multi-processor performance benchmarks completed! ✅" << std::endl;
    }
};

int main() {
    try {
        MultiprocessorPerformanceTest::runAllBenchmarks();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Performance test failed: " << e.what() << std::endl;
        return 1;
    }
}