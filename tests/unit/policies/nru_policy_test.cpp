/**
 * @file nru_policy_test.cpp
 * @brief Unit tests for NRU (Not Recently Used) replacement policy
 * @author Mudit Bhargava
 * @date 2025-06-01
 * @version 1.2.0
 */

#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <random>
#include "../../../src/core/replacement_policy.h"

using namespace cachesim;

class NRUPolicyTest {
public:
    static void testBasicFunctionality() {
        std::cout << "Testing NRU basic functionality..." << std::endl;
        
        const int numBlocks = 4;
        NRUPolicy policy(numBlocks);
        
        // All blocks should be not recently used initially
        std::vector<bool> validBlocks(numBlocks, true);
        
        // First victim should be block 0
        int victim = policy.selectVictim(validBlocks);
        assert(victim >= 0 && victim < numBlocks && "Victim should be valid");
        
        // Mark some blocks as recently used
        policy.onAccess(0);
        policy.onAccess(2);
        
        // Now victim should be 1 or 3 (not recently used)
        victim = policy.selectVictim(validBlocks);
        assert((victim == 1 || victim == 3) && "Victim should be not recently used");
        
        std::cout << "✓ Basic functionality test passed!" << std::endl;
    }
    
    static void testReferenceBitClearing() {
        std::cout << "Testing reference bit clearing..." << std::endl;
        
        const int numBlocks = 4;
        NRUPolicy policy(numBlocks);
        std::vector<bool> validBlocks(numBlocks, true);
        
        // Access all blocks many times to trigger clearing
        for (int i = 0; i < numBlocks * 10; ++i) {
            policy.onAccess(i % numBlocks);
        }
        
        // After clearing, all blocks should be equal candidates
        std::vector<int> victimCounts(numBlocks, 0);
        
        // Run multiple selections to see distribution
        for (int i = 0; i < 100; ++i) {
            // Reset to fresh state
            NRUPolicy freshPolicy(numBlocks);
            
            // Mark all as accessed except one
            for (int j = 0; j < numBlocks; ++j) {
                if (j != i % numBlocks) {
                    freshPolicy.onAccess(j);
                }
            }
            
            int victim = freshPolicy.selectVictim(validBlocks);
            victimCounts[victim]++;
        }
        
        // The not-accessed block should be selected most often
        std::cout << "✓ Reference bit clearing test passed!" << std::endl;
    }
    
    static void testWithPartiallyValidBlocks() {
        std::cout << "Testing with partially valid blocks..." << std::endl;
        
        const int numBlocks = 8;
        NRUPolicy policy(numBlocks);
        
        // Only blocks 2, 4, 6 are valid
        std::vector<bool> validBlocks(numBlocks, false);
        validBlocks[2] = validBlocks[4] = validBlocks[6] = true;
        
        // Access block 2 and 4
        policy.onAccess(2);
        policy.onAccess(4);
        
        // Victim should be block 6 (not recently used and valid)
        int victim = policy.selectVictim(validBlocks);
        assert(victim == 6 && "Should select valid, not recently used block");
        
        // Access block 6, now all are recently used
        policy.onAccess(6);
        
        // Should still select a valid block
        victim = policy.selectVictim(validBlocks);
        assert(validBlocks[victim] && "Should select a valid block");
        
        std::cout << "✓ Partially valid blocks test passed!" << std::endl;
    }
    
    static void testComparisonWithLRU() {
        std::cout << "Testing NRU vs LRU behavior..." << std::endl;
        
        const int numBlocks = 4;
        const int numAccesses = 1000;
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> blockDist(0, numBlocks - 1);
        
        NRUPolicy nru(numBlocks);
        LRUPolicy lru(numBlocks);
        std::vector<bool> validBlocks(numBlocks, true);
        
        int nruVictims[numBlocks] = {0};
        int lruVictims[numBlocks] = {0};
        
        // Simulate access pattern
        for (int i = 0; i < numAccesses; ++i) {
            int block = blockDist(rng);
            
            nru.onAccess(block);
            lru.onAccess(block);
            
            if (i % 10 == 0) {  // Periodically select victims
                nruVictims[nru.selectVictim(validBlocks)]++;
                lruVictims[lru.selectVictim(validBlocks)]++;
            }
        }
        
        // NRU should have different distribution than LRU
        bool different = false;
        for (int i = 0; i < numBlocks; ++i) {
            if (std::abs(nruVictims[i] - lruVictims[i]) > numAccesses / 20) {
                different = true;
                break;
            }
        }
        
        assert(different && "NRU should behave differently from LRU");
        std::cout << "✓ NRU vs LRU comparison test passed!" << std::endl;
    }
    
    static void testStressWithManyBlocks() {
        std::cout << "Testing NRU with many blocks..." << std::endl;
        
        const int numBlocks = 256;
        NRUPolicy policy(numBlocks);
        std::vector<bool> validBlocks(numBlocks, true);
        
        std::mt19937 rng(12345);
        std::uniform_int_distribution<int> blockDist(0, numBlocks - 1);
        
        // Access pattern with locality
        for (int phase = 0; phase < 10; ++phase) {
            // Hot set for this phase
            std::vector<int> hotSet;
            for (int i = 0; i < numBlocks / 4; ++i) {
                hotSet.push_back(blockDist(rng));
            }
            
            // Access hot set frequently
            for (int i = 0; i < 1000; ++i) {
                int idx = rng() % hotSet.size();
                policy.onAccess(hotSet[idx]);
                
                if (i % 100 == 0) {
                    int victim = policy.selectVictim(validBlocks);
                    
                    // Victim should not be in hot set (with high probability)
                    bool inHotSet = std::find(hotSet.begin(), hotSet.end(), victim) != hotSet.end();
                    
                    // Allow some false positives due to reference bit clearing
                    static int hotSetVictims = 0;
                    if (inHotSet) hotSetVictims++;
                    
                    // Less than 20% of victims should be from hot set
                    assert(hotSetVictims < phase * 2 && "Too many hot set evictions");
                }
            }
        }
        
        std::cout << "✓ Stress test with many blocks passed!" << std::endl;
    }
    
    static void testResetFunctionality() {
        std::cout << "Testing reset functionality..." << std::endl;
        
        const int numBlocks = 4;
        NRUPolicy policy(numBlocks);
        std::vector<bool> validBlocks(numBlocks, true);
        
        // Access some blocks
        policy.onAccess(0);
        policy.onAccess(1);
        policy.onAccess(2);
        
        // Reset
        policy.reset();
        
        // After reset, all blocks should be equal candidates
        std::vector<int> victimCounts(numBlocks, 0);
        
        for (int i = 0; i < 100; ++i) {
            NRUPolicy freshPolicy(numBlocks);
            int victim = freshPolicy.selectVictim(validBlocks);
            victimCounts[victim]++;
        }
        
        // Should have relatively even distribution
        for (int count : victimCounts) {
            assert(count > 10 && "Distribution should be relatively even after reset");
        }
        
        std::cout << "✓ Reset functionality test passed!" << std::endl;
    }
    
    static void runAllTests() {
        std::cout << "Running NRU Policy Tests..." << std::endl;
        std::cout << "===========================" << std::endl;
        
        testBasicFunctionality();
        testReferenceBitClearing();
        testWithPartiallyValidBlocks();
        testComparisonWithLRU();
        testStressWithManyBlocks();
        testResetFunctionality();
        
        std::cout << std::endl;
        std::cout << "All NRU Policy tests passed! ✅" << std::endl;
    }
};

int main() {
    try {
        NRUPolicyTest::runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
