/**
 * @file associativity_test.cpp
 * @brief Test that the cache correctly fills all ways and resolves conflicts
 *
 * Verifies the fix for the bug where findVictim failed to prefer empty blocks,
 * making the cache effectively 1-way regardless of configured associativity.
 */

#include "../../src/core/cache.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace cachesim;

/**
 * @brief Test that the cache correctly fills all ways before evicting.
 */
static bool testAllWaysFilled() {
  std::cout << "  Verifying all ways are filled before eviction...\n";

  // 4-way cache, 2 sets, blockSize=4 -> total 32 bytes, 8 blocks
  CacheConfig cfg(32, 4, 4);
  Cache cache(cfg);

  // Access 4 addresses that all map to set 0
  // With blockSize=4 and numSets=2, set index = (addr/4) % 2
  // Addresses 0, 8, 16, 24 -> blocks 0, 2, 4, 6 -> set indices 0, 0, 0, 0
  uint32_t addrs[] = {0, 8, 16, 24};
  for (uint32_t a : addrs) {
    cache.access(a, false);
  }

  // All 4 blocks in set 0 should be valid
  for (int way = 0; way < 4; ++way) {
    if (!cache.isBlockValid(0, way)) {
      std::cerr << "    FAIL: set 0, way " << way
                << " is not valid after 4 unique accesses to set 0\n";
      return false;
    }
  }

  // Re-access all 4 -- should all be hits
  int hitsBefore = cache.getHits();
  for (uint32_t a : addrs) {
    cache.access(a, false);
  }
  int hitsAfter = cache.getHits();

  if (hitsAfter - hitsBefore != 4) {
    std::cerr << "    FAIL: expected 4 hits on re-access, got "
              << (hitsAfter - hitsBefore) << "\n";
    return false;
  }

  std::cout << "    PASS: all 4 ways filled, 4/4 hits on re-access\n";
  return true;
}

/**
 * @brief Test that 2-way resolves conflicts that 1-way cannot.
 *
 * Creates pairs of addresses that map to the same set. In a 1-way cache
 * they ping-pong and every access misses. In a 2-way cache both fit.
 */
static bool testConflictResolution() {
  std::cout << "  Verifying associativity resolves set conflicts...\n";

  const int cacheSize = 256;
  const int blockSize = 4;

  // 1-way: 64 sets. Blocks 0 and 64 both map to set 0.
  // Repeated access to both -> every access is a conflict miss.
  CacheConfig cfg1(cacheSize, 1, blockSize);
  Cache cache1(cfg1);
  cache1.access(0, false);
  cache1.access(256, false); // block 64 -> set 0 in 1-way
  for (int i = 0; i < 100; ++i) {
    cache1.access(0, false);
    cache1.access(256, false);
  }
  double ratio1 = cache1.getHitRatio();
  std::cout << "    1-way conflict pair: hit ratio = " << (ratio1 * 100.0)
            << "%\n";

  // 2-way: 32 sets. Blocks 0 and 64 both map to set 0. Both fit in 2 ways.
  CacheConfig cfg2(cacheSize, 2, blockSize);
  Cache cache2(cfg2);
  cache2.access(0, false);
  cache2.access(256, false);
  for (int i = 0; i < 100; ++i) {
    cache2.access(0, false);
    cache2.access(256, false);
  }
  double ratio2 = cache2.getHitRatio();
  std::cout << "    2-way conflict pair: hit ratio = " << (ratio2 * 100.0)
            << "%\n";

  if (ratio2 <= ratio1) {
    std::cerr << "    FAIL: 2-way should have higher hit ratio than 1-way "
                 "for conflict pairs\n";
    return false;
  }

  std::cout << "    PASS: 2-way resolves conflicts that 1-way cannot\n";
  return true;
}

/**
 * @brief Test that N-way cache can hold N blocks in the same set.
 *
 * Verifies that a 4-way cache achieves near-perfect hit ratio when
 * repeatedly accessing 4 blocks that all map to the same set,
 * confirming that all 4 ways are being utilised.
 */
static bool testNWayHoldsNBlocks() {
  std::cout << "  Verifying 4-way cache holds 4 blocks in one set...\n";

  // 4-way cache: size=256, blockSize=4 -> numSets=16
  // Blocks 0, 16, 32, 48 all map to set 0 (blockNum % 16 == 0)
  // Addresses: 0, 64, 128, 192
  const int cacheSize = 256;
  const int blockSize = 4;
  CacheConfig cfg(cacheSize, 4, blockSize);
  Cache cache(cfg);

  uint32_t addrs[] = {0, 64, 128, 192};

  // Warmup: install all 4 blocks
  for (uint32_t a : addrs) {
    cache.access(a, false);
  }

  // Reset stats after warmup
  cache.resetStats();

  // Repeated accesses -- all should be hits
  for (int i = 0; i < 200; ++i) {
    for (uint32_t a : addrs) {
      cache.access(a, false);
    }
  }

  double ratio = cache.getHitRatio();
  std::cout << "    4-way with 4 conflicting blocks: hit ratio = "
            << (ratio * 100.0) << "%\n";

  if (ratio < 0.99) {
    std::cerr << "    FAIL: expected ~100% hit ratio, got " << (ratio * 100.0)
              << "%\n";
    return false;
  }

  std::cout << "    PASS: 4-way holds all 4 blocks, near-perfect hits\n";
  return true;
}

/**
 * @brief Verify tag and set index decomposition correctness.
 */
static bool testTagDecomposition() {
  std::cout << "  Verifying tag/set decomposition...\n";

  // 4-way cache, 16 sets, blockSize=4 -> size = 4*16*4 = 256
  CacheConfig cfg(256, 4, 4);
  Cache cache(cfg);

  // Access address 100: blockNum = 25, set = 25%16 = 9, tag = 25/16 = 1
  auto [tag, setIndex] = cache.getTagAndSet(100);
  if (setIndex != 9 || tag != 1) {
    std::cerr << "    FAIL: addr 100 -> expected set=9,tag=1, got set="
              << setIndex << ",tag=" << tag << "\n";
    return false;
  }

  // Access and verify it lands in the correct set
  cache.access(100, false);
  if (!cache.isBlockValid(9, 0)) {
    std::cerr << "    FAIL: block not installed in expected set 9\n";
    return false;
  }

  // Access same address again -- should be a hit
  bool hit = cache.access(100, false);
  if (!hit) {
    std::cerr << "    FAIL: re-access of address 100 should be a hit\n";
    return false;
  }

  std::cout << "    PASS: tag/set decomposition correct\n";
  return true;
}

int main() {
  bool allPassed = true;

  std::cout << "=== Associativity Correctness Tests ===\n\n";

  std::cout << "Test 1: Way-fill correctness\n";
  allPassed &= testAllWaysFilled();
  std::cout << "\n";

  std::cout << "Test 2: Conflict resolution (2-way vs 1-way)\n";
  allPassed &= testConflictResolution();
  std::cout << "\n";

  std::cout << "Test 3: N-way holds N blocks\n";
  allPassed &= testNWayHoldsNBlocks();
  std::cout << "\n";

  std::cout << "Test 4: Tag and set index decomposition\n";
  allPassed &= testTagDecomposition();
  std::cout << "\n";

  if (allPassed) {
    std::cout << "ALL TESTS PASSED\n";
    return 0;
  } else {
    std::cerr << "SOME TESTS FAILED\n";
    return 1;
  }
}
