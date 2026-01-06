/**
 * @file area_model.h
 * @brief Area estimation model for cache simulation
 * @author Mudit Bhargava
 * @date 2026-01-06
 * @version 1.3.0
 *
 * Implements CACTI-inspired analytical area model including:
 * - Data array (SRAM cells)
 * - Tag array
 * - Decoder circuitry
 * - Sense amplifiers
 * - Routing and interconnect
 */

#ifndef CACHESIM_AREA_MODEL_H
#define CACHESIM_AREA_MODEL_H

#include <cstdint>
#include <string>

namespace cachesim {

// Forward declaration
struct CacheConfig;

/**
 * @brief Detailed breakdown of cache area components
 */
struct AreaBreakdown {
  // Component areas (mm²)
  double dataArray = 0.0;  ///< SRAM data cells
  double tagArray = 0.0;   ///< Tag storage cells
  double decoder = 0.0;    ///< Row and column decoders
  double senseAmps = 0.0;  ///< Sense amplifiers
  double drivers = 0.0;    ///< Wordline/bitline drivers
  double routing = 0.0;    ///< Metal interconnect
  double peripheral = 0.0; ///< Control logic, buffers
  double total = 0.0;      ///< Total cache area

  // Geometry
  double aspectRatio = 1.0; ///< Width/Height ratio
  double width = 0.0;       ///< Estimated width (mm)
  double height = 0.0;      ///< Estimated height (mm)

  // Efficiency metrics
  double cellEfficiency = 0.0; ///< Data cells / total area
};

/**
 * @class AreaModel
 * @brief CACTI-inspired cache area estimation
 *
 * This model calculates silicon area based on:
 * - Technology node (determines cell size)
 * - Cache organization (size, associativity, block size)
 * - Peripheral circuit overhead
 *
 * References:
 * - CACTI 7.0 (HP Labs)
 * - "Cache Area Modeling" (Wilton & Jouppi, 1996)
 */
class AreaModel {
public:
  /**
   * @brief Calculate area breakdown for a cache configuration
   * @param cacheSize Cache size in bytes
   * @param blockSize Block size in bytes
   * @param associativity Cache associativity
   * @param tagBits Number of tag bits
   * @param techNode Technology node in nm (7, 14, 22, 32, 45)
   * @return AreaBreakdown with all component areas
   */
  [[nodiscard]] static AreaBreakdown
  calculate(uint32_t cacheSize, uint32_t blockSize, uint32_t associativity,
            uint32_t tagBits, uint32_t techNode);

  /**
   * @brief Calculate area using CacheConfig structure
   * @param config Cache configuration
   * @param techNode Technology node in nm
   * @return AreaBreakdown with all component areas
   */
  [[nodiscard]] static AreaBreakdown calculate(const CacheConfig &config,
                                               uint32_t techNode);

  /**
   * @brief Compare two configurations on area
   * @param breakdown1 First configuration area
   * @param breakdown2 Second configuration area
   * @return Relative area difference (positive = config1 larger)
   */
  [[nodiscard]] static double compare(const AreaBreakdown &breakdown1,
                                      const AreaBreakdown &breakdown2);

  /**
   * @brief Generate human-readable area report
   */
  [[nodiscard]] static std::string
  generateReport(const AreaBreakdown &breakdown, uint32_t techNode);

  /**
   * @brief Estimate optimal aspect ratio for cache layout
   * @param numSets Number of sets
   * @param associativity Cache associativity
   * @param blockSize Block size in bytes
   * @return Optimal aspect ratio (width/height)
   */
  [[nodiscard]] static double estimateAspectRatio(uint32_t numSets,
                                                  uint32_t associativity,
                                                  uint32_t blockSize);

private:
  // Internal calculation methods
  static double calculateDataArrayArea(uint32_t cacheSize, uint32_t techNode);

  static double calculateTagArrayArea(uint32_t numSets, uint32_t associativity,
                                      uint32_t tagBits, uint32_t techNode);

  static double calculateDecoderArea(uint32_t numSets, uint32_t associativity,
                                     uint32_t techNode);

  static double calculateSenseAmpArea(uint32_t blockSize,
                                      uint32_t associativity,
                                      uint32_t techNode);

  static double calculateDriverArea(uint32_t numSets, uint32_t blockSize,
                                    uint32_t techNode);

  static double calculateRoutingArea(double totalCellArea,
                                     uint32_t associativity);
};

} // namespace cachesim

#endif // CACHESIM_AREA_MODEL_H
