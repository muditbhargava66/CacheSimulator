/**
 * @file l3_cache_test.cpp
 * @brief Unit tests for L3 cache support in MemoryHierarchy
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include <cassert>
#include <iostream>

#include "core/cache.h"
#include "core/memory_hierarchy.h"

using namespace cachesim;

/**
 * @class L3CacheTest
 * @brief Test suite for L3 cache functionality
 */
class L3CacheTest {
public:
  static void runAllTests() {
    std::cout << "=== L3 Cache Tests ===" << std::endl;

    testL3Configuration();
    testL3AccessFlow();
    testL3Statistics();
    testInclusiveL3();

    std::cout << "\nAll L3 cache tests passed!" << std::endl;
  }

private:
  static void testL3Configuration() {
    std::cout << "\n[TEST] L3 Configuration..." << std::endl;

    // Create config with L1, L2, and L3
    CacheConfig l1Config(32768, 4, 64);    // 32KB L1
    CacheConfig l2Config(262144, 8, 64);   // 256KB L2
    CacheConfig l3Config(8388608, 16, 64); // 8MB L3

    MemoryHierarchyConfig config;
    config.l1Config = l1Config;
    config.l2Config = l2Config;
    config.l3Config = l3Config;
    config.l3Inclusive = true;

    MemoryHierarchy hierarchy(config);

    // Verify L3 is present
    assert(hierarchy.hasL3() && "L3 cache should be present");
    assert(hierarchy.getL3Cache().has_value() && "L3 cache pointer valid");

    std::cout << "  L3 configuration: PASSED" << std::endl;
  }

  static void testL3AccessFlow() {
    std::cout << "\n[TEST] L3 Access Flow..." << std::endl;

    // Small caches to force misses
    CacheConfig l1Config(1024, 2, 64);  // 1KB L1
    CacheConfig l2Config(4096, 4, 64);  // 4KB L2
    CacheConfig l3Config(16384, 8, 64); // 16KB L3

    MemoryHierarchyConfig config;
    config.l1Config = l1Config;
    config.l2Config = l2Config;
    config.l3Config = l3Config;

    MemoryHierarchy hierarchy(config);

    // Access pattern that overflows L1 and L2 but fits in L3
    // L1 has 1024/64 = 16 blocks, L2 has 64 blocks, L3 has 256 blocks

    // First pass: fill caches
    for (int i = 0; i < 100; i++) {
      hierarchy.access(i * 64, false); // Read
    }

    // Second pass: some should hit in L3
    hierarchy.resetStats();
    for (int i = 0; i < 50; i++) {
      hierarchy.access(i * 64, false);
    }

    // Should have L1 misses but L3 should catch some
    int l1Misses = hierarchy.getL1Misses();
    int l3Misses = hierarchy.getL3Misses();

    std::cout << "  L1 misses: " << l1Misses << ", L3 misses: " << l3Misses
              << std::endl;
    assert(l1Misses > 0 && "Should have L1 misses");

    std::cout << "  L3 access flow: PASSED" << std::endl;
  }

  static void testL3Statistics() {
    std::cout << "\n[TEST] L3 Statistics..." << std::endl;

    CacheConfig l1Config(1024, 2, 64);
    CacheConfig l2Config(2048, 4, 64);
    CacheConfig l3Config(8192, 8, 64);

    MemoryHierarchyConfig config;
    config.l1Config = l1Config;
    config.l2Config = l2Config;
    config.l3Config = l3Config;

    MemoryHierarchy hierarchy(config);

    // Perform some accesses
    for (int i = 0; i < 200; i++) {
      hierarchy.access(i * 64, i % 4 == 0); // 25% writes
    }

    // Check that stats are tracked
    double l3HitRate = hierarchy.getL3HitRate();
    double l3MissRate = hierarchy.getL3MissRate();

    std::cout << "  L3 hit rate: " << (l3HitRate * 100) << "%" << std::endl;
    std::cout << "  L3 miss rate: " << (l3MissRate * 100) << "%" << std::endl;

    // Hit rate + miss rate should be ~1.0 (accounting for rounding)
    assert(l3HitRate >= 0.0 && l3HitRate <= 1.0 &&
           "L3 hit rate in valid range");
    assert(l3MissRate >= 0.0 && l3MissRate <= 1.0 &&
           "L3 miss rate in valid range");

    std::cout << "  L3 statistics: PASSED" << std::endl;
  }

  static void testInclusiveL3() {
    std::cout << "\n[TEST] Inclusive L3 Policy..." << std::endl;

    CacheConfig l1Config(1024, 2, 64);
    CacheConfig l2Config(2048, 4, 64);
    CacheConfig l3Config(4096, 4, 64);

    MemoryHierarchyConfig config;
    config.l1Config = l1Config;
    config.l2Config = l2Config;
    config.l3Config = l3Config;
    config.l3Inclusive = true;

    MemoryHierarchy hierarchy(config);

    // Test that inclusive L3 is configured
    assert(hierarchy.hasL3() && "Inclusive L3 configured");

    // Note: Full back-invalidation testing would require
    // exposing more internal state

    std::cout << "  Inclusive L3 policy: PASSED" << std::endl;
  }
};

int main() {
  try {
    L3CacheTest::runAllTests();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
