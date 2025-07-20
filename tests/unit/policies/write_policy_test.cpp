/**
 * @file write_policy_test.cpp
 * @brief Unit tests for write policies including no-write-allocate
 * @author Mudit Bhargava
 * @date 2025-06-01
 * @version 1.2.0
 */

#include <iostream>
#include <cassert>
#include <memory>
#include "../../../src/core/write_policy.h"
#include "../../../src/core/cache.h"

using namespace cachesim;

class WritePolicyTest {
public:
    static void testWriteThroughNoAllocate() {
        std::cout << "Testing write-through no-allocate policy..." << std::endl;
        
        // Create cache with write-through no-allocate
        CacheConfig config(32768, 4, 64);  // 32KB, 4-way, 64B blocks
        config.writePolicy = WritePolicy::WriteThrough;
        
        Cache cache(config);
        
        // Write miss should not allocate
        uint32_t missAddr = 0x1000;
        bool hit = cache.access(missAddr, true);  // write
        assert(!hit && "First access should miss");
        
        // Subsequent read should still miss (no allocation occurred)
        hit = cache.access(missAddr, false);  // read
        assert(!hit && "Read should miss since write didn't allocate");
        
        // Read miss should allocate
        hit = cache.access(missAddr, false);  // read again
        assert(!hit && "Still a miss on first read");
        
        // Now write should hit
        hit = cache.access(missAddr, true);  // write
        assert(hit && "Write should hit after read allocation");
        
        std::cout << "✓ Write-through no-allocate test passed!" << std::endl;
    }
    
    static void testWriteBackNoAllocate() {
        std::cout << "Testing write-back no-allocate policy..." << std::endl;
        
        auto policy = std::make_unique<WriteBackNoAllocatePolicy>();
        
        assert(!policy->requiresAllocation() && "Should not require allocation");
        assert(policy->requiresWriteback() && "Should require writeback");
        assert(policy->getName() == "WriteBack+NoWriteAllocate");
        
        std::cout << "✓ Write-back no-allocate test passed!" << std::endl;
    }
    
    static void testWriteCombiningBuffer() {
        std::cout << "Testing write combining buffer..." << std::endl;
        
        WriteCombiningBuffer buffer(4);  // 4-entry buffer
        
        // Test combining
        assert(buffer.tryWrite(0x1000) && "First write should succeed");
        assert(buffer.tryWrite(0x1000) && "Same address should combine");
        assert(buffer.getOccupancy() == 1 && "Should still have 1 entry");
        
        // Fill buffer
        assert(buffer.tryWrite(0x2000) && "Second address");
        assert(buffer.tryWrite(0x3000) && "Third address");
        assert(buffer.tryWrite(0x4000) && "Fourth address");
        assert(buffer.getOccupancy() == 4 && "Buffer should be full");
        
        // Next write should evict oldest
        assert(!buffer.tryWrite(0x5000) && "Should indicate eviction");
        
        // Test flush
        auto flushed = buffer.flush();
        assert(flushed.size() == 4 && "Should flush 4 addresses");
        assert(buffer.getOccupancy() == 0 && "Buffer should be empty after flush");
        
        std::cout << "✓ Write combining buffer test passed!" << std::endl;
    }
    
    static void testWritePolicyFactory() {
        std::cout << "Testing write policy factory..." << std::endl;
        
        // Test all combinations
        auto wt_wa = WritePolicyFactory::create(
            WriteUpdatePolicy::WriteThrough,
            WriteAllocationPolicy::WriteAllocate);
        assert(wt_wa->getName() == "WriteThrough+WriteAllocate");
        
        auto wt_nwa = WritePolicyFactory::create(
            WriteUpdatePolicy::WriteThrough,
            WriteAllocationPolicy::NoWriteAllocate);
        assert(wt_nwa->getName() == "WriteThrough+NoWriteAllocate");
        
        auto wb_wa = WritePolicyFactory::create(
            WriteUpdatePolicy::WriteBack,
            WriteAllocationPolicy::WriteAllocate);
        assert(wb_wa->getName() == "WriteBack+WriteAllocate");
        
        auto wb_nwa = WritePolicyFactory::create(
            WriteUpdatePolicy::WriteBack,
            WriteAllocationPolicy::NoWriteAllocate);
        assert(wb_nwa->getName() == "WriteBack+NoWriteAllocate");
        
        std::cout << "✓ Write policy factory test passed!" << std::endl;
    }
    
    static void testWritePolicyStatistics() {
        std::cout << "Testing write policy statistics..." << std::endl;
        
        // Create two caches with different policies
        CacheConfig config1(8192, 2, 64);  // 8KB, 2-way
        config1.writePolicy = WritePolicy::WriteThrough;
        
        CacheConfig config2(8192, 2, 64);
        config2.writePolicy = WritePolicy::WriteBack;
        
        Cache writeThrough(config1);
        Cache writeBack(config2);
        
        // Generate write-heavy workload
        const int numAccesses = 1000;
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> addrDist(0x1000, 0x4000);
        
        for (int i = 0; i < numAccesses; ++i) {
            uint32_t addr = addrDist(rng) & ~0x3F;  // Align to block
            bool isWrite = (i % 3) != 0;  // 66% writes
            
            writeThrough.access(addr, isWrite);
            writeBack.access(addr, isWrite);
        }
        
        // Write-through should have more write operations
        auto wtWrites = writeThrough.getWrites();
        auto wbWrites = writeBack.getWrites();
        auto wbWriteBacks = writeBack.getWriteBacks();
        
        std::cout << "  Write-through writes: " << wtWrites << std::endl;
        std::cout << "  Write-back writes: " << wbWrites << std::endl;
        std::cout << "  Write-back writebacks: " << wbWriteBacks << std::endl;
        
        assert(wtWrites > 0 && "Should have writes");
        assert(wbWriteBacks < wtWrites && "Write-back should have fewer memory writes");
        
        std::cout << "✓ Write policy statistics test passed!" << std::endl;
    }
    
    static void testMixedReadWritePattern() {
        std::cout << "Testing mixed read/write patterns..." << std::endl;
        
        WriteCombiningBuffer buffer(8);
        
        // Simulate realistic mixed pattern
        std::vector<uint32_t> addresses = {
            0x1000, 0x1040, 0x1080, 0x10C0,  // Sequential
            0x2000, 0x2000, 0x2000, 0x2000,  // Repeated writes
            0x3000, 0x3100, 0x3200, 0x3300   // Strided
        };
        
        int combines = 0;
        int evictions = 0;
        
        for (auto addr : addresses) {
            bool success = buffer.tryWrite(addr);
            if (!success) evictions++;
            
            // Check if it was a combine
            auto occupancyBefore = buffer.getOccupancy();
            buffer.tryWrite(addr);  // Write again
            if (buffer.getOccupancy() == occupancyBefore) {
                combines++;
            }
        }
        
        std::cout << "  Combines: " << combines << std::endl;
        std::cout << "  Evictions: " << evictions << std::endl;
        
        assert(combines > 0 && "Should have some combines");
        
        std::cout << "✓ Mixed read/write pattern test passed!" << std::endl;
    }
    
    static void runAllTests() {
        std::cout << "Running Write Policy Tests..." << std::endl;
        std::cout << "=============================" << std::endl;
        
        testWriteThroughNoAllocate();
        testWriteBackNoAllocate();
        testWriteCombiningBuffer();
        testWritePolicyFactory();
        testWritePolicyStatistics();
        testMixedReadWritePattern();
        
        std::cout << std::endl;
        std::cout << "All Write Policy tests passed! ✅" << std::endl;
    }
};

int main() {
    try {
        WritePolicyTest::runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
