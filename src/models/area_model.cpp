/**
 * @file area_model.cpp
 * @brief Implementation of CACTI-inspired area model
 * @author Mudit Bhargava
 * @date 2026-01-06
 * @version 1.3.0
 */

#include "area_model.h"
#include "../core/cache.h"
#include "power_constants.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace cachesim {

AreaBreakdown AreaModel::calculate(uint32_t cacheSize, uint32_t blockSize,
                                   uint32_t associativity, uint32_t tagBits,
                                   uint32_t techNode) {
  AreaBreakdown breakdown;

  // Calculate number of sets
  uint32_t numSets = cacheSize / (blockSize * associativity);

  // Calculate component areas
  breakdown.dataArray = calculateDataArrayArea(cacheSize, techNode);
  breakdown.tagArray =
      calculateTagArrayArea(numSets, associativity, tagBits, techNode);
  breakdown.decoder = calculateDecoderArea(numSets, associativity, techNode);
  breakdown.senseAmps =
      calculateSenseAmpArea(blockSize, associativity, techNode);
  breakdown.drivers = calculateDriverArea(numSets, blockSize, techNode);

  // Calculate cell area (data + tag)
  double cellArea = breakdown.dataArray + breakdown.tagArray;

  // Routing area scales with associativity and total cell area
  breakdown.routing = calculateRoutingArea(cellArea, associativity);

  // Peripheral circuits (~5% of total)
  double subtotal = breakdown.dataArray + breakdown.tagArray +
                    breakdown.decoder + breakdown.senseAmps +
                    breakdown.drivers + breakdown.routing;
  breakdown.peripheral = subtotal * 0.05;

  // Total area
  breakdown.total = subtotal + breakdown.peripheral;

  // Calculate efficiency
  breakdown.cellEfficiency = breakdown.dataArray / breakdown.total;

  // Estimate aspect ratio and dimensions
  breakdown.aspectRatio =
      estimateAspectRatio(numSets, associativity, blockSize);
  breakdown.height = std::sqrt(breakdown.total / breakdown.aspectRatio);
  breakdown.width = breakdown.height * breakdown.aspectRatio;

  return breakdown;
}

AreaBreakdown AreaModel::calculate(const CacheConfig &config,
                                   uint32_t techNode) {
  // Calculate tag bits (assume 32-bit address space)
  uint32_t numSets = config.size / (config.blockSize * config.associativity);
  uint32_t indexBits = 0;
  uint32_t temp = numSets;
  while (temp > 1) {
    indexBits++;
    temp >>= 1;
  }
  uint32_t offsetBits = 0;
  temp = config.blockSize;
  while (temp > 1) {
    offsetBits++;
    temp >>= 1;
  }
  uint32_t tagBits = 32 - indexBits - offsetBits;

  return calculate(config.size, config.blockSize, config.associativity, tagBits,
                   techNode);
}

double AreaModel::compare(const AreaBreakdown &breakdown1,
                          const AreaBreakdown &breakdown2) {
  if (breakdown2.total == 0)
    return 0.0;
  return (breakdown1.total - breakdown2.total) / breakdown2.total * 100.0;
}

double AreaModel::calculateDataArrayArea(uint32_t cacheSize,
                                         uint32_t techNode) {
  const auto &tech = power::getTechParams(techNode);

  // Data array = cacheSize × 8 bits × SRAM cell area
  uint64_t numBits = static_cast<uint64_t>(cacheSize) * 8;
  return numBits * tech.sramCellArea;
}

double AreaModel::calculateTagArrayArea(uint32_t numSets,
                                        uint32_t associativity,
                                        uint32_t tagBits, uint32_t techNode) {
  const auto &tech = power::getTechParams(techNode);

  // Tag array = numSets × associativity × tagBits × SRAM cell area
  // Plus valid bit and dirty bit per entry
  uint64_t bitsPerEntry = tagBits + 2; // +valid +dirty
  uint64_t totalBits =
      static_cast<uint64_t>(numSets) * associativity * bitsPerEntry;

  return totalBits * tech.sramCellArea;
}

double AreaModel::calculateDecoderArea(uint32_t numSets, uint32_t associativity,
                                       uint32_t techNode) {
  const auto &tech = power::getTechParams(techNode);

  // Decoder area scales with log2(sets) × transistor width
  uint32_t indexBits = 0;
  uint32_t temp = numSets;
  while (temp > 1) {
    indexBits++;
    temp >>= 1;
  }

  // Row decoder: ~2^index gates of minimum size
  // Column decoder: ~associativity gates
  double gateArea =
      tech.transistorWidth * tech.transistorWidth * 1e-12; // nm² to mm²
  double rowDecoderArea =
      numSets * gateArea * 6; // 6 transistors per decoder gate
  double colDecoderArea = associativity * gateArea * 6;

  return rowDecoderArea + colDecoderArea;
}

double AreaModel::calculateSenseAmpArea(uint32_t blockSize,
                                        uint32_t associativity,
                                        uint32_t techNode) {
  const auto &tech = power::getTechParams(techNode);

  // One sense amp per bitline pair per way
  // Each sense amp is ~20-30 transistors
  uint32_t bitsPerBlock = blockSize * 8;
  uint64_t numSenseAmps = static_cast<uint64_t>(bitsPerBlock) * associativity;

  double senseAmpTransistors = 25;
  double transistorArea =
      tech.transistorWidth * tech.transistorWidth * 1e-12; // mm²

  return numSenseAmps * senseAmpTransistors * transistorArea;
}

double AreaModel::calculateDriverArea(uint32_t numSets, uint32_t blockSize,
                                      uint32_t techNode) {
  const auto &tech = power::getTechParams(techNode);

  // Wordline drivers: one per set
  // Bitline drivers: one per column
  uint32_t numBitlines = blockSize * 8 * 2; // Differential
  uint32_t totalDrivers = numSets + numBitlines;

  // Each driver is ~10-15 transistors (inverter chain)
  double driverTransistors = 12;
  double transistorArea =
      tech.transistorWidth * tech.transistorWidth * 1e-12; // mm²

  return totalDrivers * driverTransistors * transistorArea;
}

double AreaModel::calculateRoutingArea(double totalCellArea,
                                       uint32_t associativity) {
  // Routing overhead based on CACTI analysis
  // Higher associativity requires more routing
  double baseOverhead = power::AreaOverheadFactors::ROUTING_OVERHEAD;
  double assocFactor =
      1.0 + (associativity - 1) * 0.02; // 2% per additional way

  return totalCellArea * baseOverhead * assocFactor;
}

double AreaModel::estimateAspectRatio(uint32_t numSets, uint32_t associativity,
                                      uint32_t blockSize) {
  // Optimal aspect ratio minimizes wire delays
  // Based on CACTI heuristics: sqrt(rows/cols) tends toward 1:1
  double rows = numSets;
  double cols = associativity * blockSize * 8;

  // Target approximately square with slight preference for width
  double rawRatio = std::sqrt(cols / rows);

  // Clamp to reasonable range [0.5, 4.0]
  return std::clamp(rawRatio, 0.5, 4.0);
}

std::string AreaModel::generateReport(const AreaBreakdown &breakdown,
                                      uint32_t techNode) {
  std::ostringstream oss;

  oss << std::fixed << std::setprecision(6);

  oss << "\n╔═══════════════════════════════════════════════════════════╗\n";
  oss << "║                  Cache Area Analysis                       ║\n";
  oss << "╠════════════════════════════════════════════════════════════╣\n";

  oss << "║ Technology Node:        " << std::setw(8) << techNode << " nm"
      << std::setw(22) << "║\n";

  oss << "╠═══════════════════════════════════════════════════════════╣\n";
  oss << "║ Area Breakdown                                            ║\n";
  oss << "╠═══════════════════════════════════════════════════════════╣\n";

  oss << std::setprecision(6);
  oss << "║ Data Array:             " << std::setw(10) << breakdown.dataArray
      << " mm²" << std::setw(17) << "║\n";
  oss << "║ Tag Array:              " << std::setw(10) << breakdown.tagArray
      << " mm²" << std::setw(17) << "║\n";
  oss << "║ Decoders:               " << std::setw(10) << breakdown.decoder
      << " mm²" << std::setw(17) << "║\n";
  oss << "║ Sense Amplifiers:       " << std::setw(10) << breakdown.senseAmps
      << " mm²" << std::setw(17) << "║\n";
  oss << "║ Drivers:                " << std::setw(10) << breakdown.drivers
      << " mm²" << std::setw(17) << "║\n";
  oss << "║ Routing:                " << std::setw(10) << breakdown.routing
      << " mm²" << std::setw(17) << "║\n";
  oss << "║ Peripheral:             " << std::setw(10) << breakdown.peripheral
      << " mm²" << std::setw(17) << "║\n";

  oss << "╠═══════════════════════════════════════════════════════════╣\n";
  oss << std::setprecision(4);
  oss << "║ TOTAL AREA:             " << std::setw(10) << breakdown.total
      << " mm²" << std::setw(17) << "║\n";

  oss << "╠═══════════════════════════════════════════════════════════╣\n";
  oss << "║ Layout Geometry                                            ║\n";
  oss << "╠═══════════════════════════════════════════════════════════╣\n";

  oss << std::setprecision(3);
  oss << "║ Width × Height:         " << std::setw(5) << breakdown.width
      << " × " << std::setw(5) << breakdown.height << " mm" << std::setw(13)
      << "║\n";
  oss << "║ Aspect Ratio:           " << std::setw(10) << breakdown.aspectRatio
      << std::setw(21) << "║\n";

  oss << std::setprecision(1);
  oss << "║ Cell Efficiency:        " << std::setw(10)
      << (breakdown.cellEfficiency * 100) << " %" << std::setw(20) << "║\n";

  oss << "╚═══════════════════════════════════════════════════════════╝\n";

  return oss.str();
}

} // namespace cachesim
