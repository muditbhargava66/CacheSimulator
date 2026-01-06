/**
 * @file power_constants.h
 * @brief Technology-specific constants for power and area modeling
 * @author Mudit Bhargava
 * @date 2026-01-06
 * @version 1.3.0
 *
 * Constants derived from CACTI 7.0 and academic literature.
 * Values are approximate and intended for educational/analytical purposes.
 */

#ifndef CACHESIM_POWER_CONSTANTS_H
#define CACHESIM_POWER_CONSTANTS_H

#include <cstdint>
#include <unordered_map>

namespace cachesim {
namespace power {

/**
 * @brief Supported technology nodes in nanometers
 */
enum class TechNode : uint32_t {
  NM_7 = 7,
  NM_14 = 14,
  NM_22 = 22,
  NM_32 = 32,
  NM_45 = 45
};

/**
 * @brief Technology parameters for power/area calculations
 *
 * Values based on CACTI 7.0 and published research:
 * - CACTI: An Integrated Cache and Memory Access Time, Cycle Time, Area,
 *   Leakage, and Dynamic Power Model (HP Labs)
 * - ITRS Technology Roadmap
 */
struct TechParams {
  double sramCellArea;        // mm² per bit (6T SRAM cell)
  double subthresholdI;       // Subthreshold leakage current (A/transistor)
  double gateLeakageI;        // Gate leakage current (A/transistor)
  double wireCapacitance;     // Wire capacitance (fF/µm)
  double transistorWidth;     // Minimum transistor width (nm)
  double vddNominal;          // Nominal supply voltage (V)
  double bitlineCap;          // Bitline capacitance (fF)
  double wordlineCap;         // Wordline capacitance (fF)
  double senseAmpEnergy;      // Sense amplifier energy (fJ)
  double decoderEnergyPerBit; // Decoder energy per bit (fJ)
};

/**
 * @brief Technology parameters lookup table
 *
 * CACTI 7.0 Reference Values (45nm baseline):
 * - SRAM cell: ~0.346 µm² at 45nm
 * - Vdd: 1.0V at 45nm, scales down with technology
 * - Leakage increases ~2x per generation
 */
inline const std::unordered_map<uint32_t, TechParams> TECH_PARAMS = {
    // 45nm (baseline - most validated)
    {45,
     {
         .sramCellArea = 0.346e-6, // 0.346 µm² = ~346 nm²
         .subthresholdI = 1.0e-10, // 100 pA/transistor
         .gateLeakageI = 1.0e-11,  // 10 pA/transistor
         .wireCapacitance = 0.2,   // 0.2 fF/µm
         .transistorWidth = 45,
         .vddNominal = 1.0,
         .bitlineCap = 0.25,         // fF
         .wordlineCap = 0.15,        // fF
         .senseAmpEnergy = 0.05,     // fJ per sense
         .decoderEnergyPerBit = 0.01 // fJ per bit decoded
     }},

    // 32nm
    {32,
     {.sramCellArea = 0.171e-6, // ~0.5x of 45nm
      .subthresholdI = 2.0e-10, // ~2x leakage
      .gateLeakageI = 2.0e-11,
      .wireCapacitance = 0.22,
      .transistorWidth = 32,
      .vddNominal = 0.9,
      .bitlineCap = 0.20,
      .wordlineCap = 0.12,
      .senseAmpEnergy = 0.035,
      .decoderEnergyPerBit = 0.007}},

    // 22nm
    {22,
     {.sramCellArea = 0.092e-6, // ~0.5x of 32nm
      .subthresholdI = 5.0e-10, // higher leakage
      .gateLeakageI = 4.0e-11,
      .wireCapacitance = 0.25,
      .transistorWidth = 22,
      .vddNominal = 0.8,
      .bitlineCap = 0.15,
      .wordlineCap = 0.10,
      .senseAmpEnergy = 0.025,
      .decoderEnergyPerBit = 0.005}},

    // 14nm (FinFET - reduced leakage)
    {14,
     {.sramCellArea = 0.050e-6, // ~0.5x of 22nm
      .subthresholdI = 3.0e-10, // FinFET reduces leakage
      .gateLeakageI = 2.0e-11,
      .wireCapacitance = 0.28,
      .transistorWidth = 14,
      .vddNominal = 0.75,
      .bitlineCap = 0.12,
      .wordlineCap = 0.08,
      .senseAmpEnergy = 0.018,
      .decoderEnergyPerBit = 0.003}},

    // 7nm (FinFET)
    {7,
     {.sramCellArea = 0.027e-6, // ~0.5x of 14nm
      .subthresholdI = 4.0e-10, // still relatively low for FinFET
      .gateLeakageI = 3.0e-11,
      .wireCapacitance = 0.32, // higher relative wire R/C
      .transistorWidth = 7,
      .vddNominal = 0.65,
      .bitlineCap = 0.10,
      .wordlineCap = 0.06,
      .senseAmpEnergy = 0.012,
      .decoderEnergyPerBit = 0.002}}};

/**
 * @brief Area overhead factors for cache components
 * Based on CACTI breakdown analysis
 */
struct AreaOverheadFactors {
  static constexpr double TAG_OVERHEAD = 0.15;       // Tag storage ~15% of data
  static constexpr double DECODER_OVERHEAD = 0.08;   // Row/col decoders ~8%
  static constexpr double SENSE_AMP_OVERHEAD = 0.05; // Sense amps ~5%
  static constexpr double DRIVER_OVERHEAD =
      0.06; // Wordline/bitline drivers ~6%
  static constexpr double ROUTING_OVERHEAD = 0.12; // Interconnect ~12%
  static constexpr double PERIPHERAL_TOTAL = 0.46; // Total overhead
};

/**
 * @brief Power modeling constants
 */
struct PowerConstants {
  static constexpr double WRITE_ENERGY_FACTOR =
      1.3; // Write ~30% more than read
  static constexpr double PREFETCH_ENERGY_FACTOR =
      0.8; // Prefetch slightly cheaper
  static constexpr double TEMP_COEFFICIENT =
      100.0; // Temperature scaling factor (K)
  static constexpr double REFERENCE_TEMP = 300.0; // Reference temperature (K)
  static constexpr int TRANSISTORS_PER_SRAM_CELL = 6; // 6T SRAM
  static constexpr double ACTIVITY_FACTOR = 0.1; // Typical switching activity
};

/**
 * @brief Get technology parameters for a given node
 */
inline const TechParams &getTechParams(uint32_t techNode) {
  auto it = TECH_PARAMS.find(techNode);
  if (it != TECH_PARAMS.end()) {
    return it->second;
  }
  // Default to 45nm if not found
  return TECH_PARAMS.at(45);
}

/**
 * @brief Check if a technology node is supported
 */
inline bool isSupportedTechNode(uint32_t techNode) {
  return TECH_PARAMS.find(techNode) != TECH_PARAMS.end();
}

} // namespace power
} // namespace cachesim

#endif // CACHESIM_POWER_CONSTANTS_H
