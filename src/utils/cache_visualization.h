/**
 * @file cache_visualization.h
 * @brief Cache state visualization utilities
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/cache.h"

namespace cachesim {

/**
 * @struct CacheBlockState
 * @brief Represents the state of a single cache block for visualization
 */
struct CacheBlockState {
  uint64_t address;     // Memory address
  uint64_t tag;         // Tag bits
  uint32_t set;         // Set index
  uint32_t way;         // Way within set
  bool valid;           // Valid bit
  bool dirty;           // Dirty bit
  uint32_t accessCount; // Number of times this block was accessed
  uint64_t lastAccess;  // Timestamp of last access
  bool prefetched;      // Whether this block was prefetched
};

/**
 * @class CacheVisualization
 * @brief Static class for cache state visualization utilities
 */
class CacheVisualization {
public:
  /**
   * @brief Extract cache state from a cache object
   * @param cache Reference to the cache object
   * @param cacheLevel Cache level identifier (for display purposes)
   * @return Vector of cache block states
   */
  static std::vector<CacheBlockState> extractCacheState(const Cache &cache,
                                                        int cacheLevel = 1);

  /**
   * @brief Create ASCII visualization of cache state
   * @param blockStates Vector of cache block states
   * @param cache Reference to cache for configuration info
   * @param maxBlocks Maximum number of blocks to display (0 for all)
   * @param useColors Whether to use ANSI colors
   * @param cacheLevel Cache level for display
   * @return String containing ASCII visualization
   */
  static std::string
  createVisualization(const std::vector<CacheBlockState> &blockStates,
                      const Cache &cache, uint32_t maxBlocks = 16,
                      bool useColors = true, int cacheLevel = 1);

  /**
   * @brief Create a simple summary visualization
   * @param cache Reference to the cache
   * @param cacheLevel Cache level identifier
   * @return Summary string
   */
  static std::string createSummary(const Cache &cache, int cacheLevel = 1);
};

} // namespace cachesim
