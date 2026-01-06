/**
 * @file power_model.h
 * @brief Power modeling for cache simulation
 * @author Mudit Bhargava
 * @date 2026-01-06
 * @version 1.3.0
 *
 * Implements CACTI-inspired analytical power model including:
 * - Dynamic read/write energy
 * - Static (leakage) power
 * - Temperature-dependent modeling
 * - Technology scaling (7nm-45nm)
 */

#ifndef CACHESIM_POWER_MODEL_H
#define CACHESIM_POWER_MODEL_H

#include <cmath>
#include <cstdint>
#include <string>


namespace cachesim {

/**
 * @brief Configuration for power modeling
 */
struct PowerConfig {
  double vdd = 1.0;           ///< Supply voltage (V)
  uint32_t techNode = 45;     ///< Technology node (nm): 7, 14, 22, 32, 45
  double frequency = 2.0e9;   ///< Operating frequency (Hz)
  double temperature = 350.0; ///< Temperature (K), default ~77°C
  bool enabled = true;        ///< Enable power tracking

  // Cache parameters (set during initialization)
  uint32_t cacheSize = 32768; ///< Cache size in bytes
  uint32_t blockSize = 64;    ///< Block size in bytes
  uint32_t associativity = 4; ///< Cache associativity
  uint32_t tagBits = 20;      ///< Number of tag bits
};

/**
 * @brief Power and energy statistics
 */
struct PowerStats {
  // Per-access energy (picojoules)
  double dynamicReadEnergy = 0.0;  ///< Energy per read access (pJ)
  double dynamicWriteEnergy = 0.0; ///< Energy per write access (pJ)

  // Power components (milliwatts)
  double leakagePower = 0.0;        ///< Static/leakage power (mW)
  double averageDynamicPower = 0.0; ///< Average dynamic power (mW)
  double totalPower = 0.0;          ///< Total power (mW)

  // Accumulated energy (nanojoules)
  double totalReadEnergy = 0.0;    ///< Total read energy (nJ)
  double totalWriteEnergy = 0.0;   ///< Total write energy (nJ)
  double totalDynamicEnergy = 0.0; ///< Total dynamic energy (nJ)
  double totalLeakageEnergy = 0.0; ///< Total leakage energy (nJ)
  double totalEnergy = 0.0;        ///< Total energy consumed (nJ)

  // Access counts
  uint64_t readCount = 0;  ///< Number of read accesses
  uint64_t writeCount = 0; ///< Number of write accesses
  uint64_t hitCount = 0;   ///< Number of cache hits
  uint64_t missCount = 0;  ///< Number of cache misses

  // Derived metrics
  double energyPerAccess = 0.0;    ///< Average energy per access (pJ)
  double energyDelayProduct = 0.0; ///< EDP metric (pJ × ns)

  // Simulation time
  double simulationTimeNs = 0.0; ///< Total simulation time (ns)
};

/**
 * @class PowerModel
 * @brief CACTI-inspired analytical power model for caches
 *
 * This model calculates:
 * - Dynamic energy: Based on capacitance, voltage, and switching activity
 * - Leakage power: Based on transistor count, temperature, and technology
 * - Area-dependent effects: Scales with cache size and organization
 *
 * References:
 * - CACTI 7.0 (HP Labs)
 * - "An Integrated Cache Timing, Power, and Area Model" (Thoziyoor et al.)
 */
class PowerModel {
public:
  /**
   * @brief Construct power model with configuration
   * @param config Power configuration parameters
   */
  explicit PowerModel(const PowerConfig &config);

  /**
   * @brief Default constructor (disabled power tracking)
   */
  PowerModel();

  /**
   * @brief Configure the power model
   * @param config Power configuration parameters
   */
  void configure(const PowerConfig &config);

  /**
   * @brief Record a cache access
   * @param isWrite True for write, false for read
   * @param isHit True for cache hit, false for miss
   */
  void recordAccess(bool isWrite, bool isHit);

  /**
   * @brief Record a prefetch access
   */
  void recordPrefetch();

  /**
   * @brief Update simulation time (for leakage calculation)
   * @param elapsedNs Elapsed time in nanoseconds
   */
  void updateSimulationTime(double elapsedNs);

  /**
   * @brief Get current power statistics
   * @return PowerStats with all computed metrics
   */
  [[nodiscard]] PowerStats getStats() const;

  /**
   * @brief Reset all counters and accumulated energy
   */
  void reset();

  /**
   * @brief Check if power modeling is enabled
   */
  [[nodiscard]] bool isEnabled() const { return config_.enabled; }

  /**
   * @brief Get per-read energy in picojoules
   */
  [[nodiscard]] double getReadEnergy() const { return dynamicReadEnergy_; }

  /**
   * @brief Get per-write energy in picojoules
   */
  [[nodiscard]] double getWriteEnergy() const { return dynamicWriteEnergy_; }

  /**
   * @brief Get leakage power in milliwatts
   */
  [[nodiscard]] double getLeakagePower() const { return leakagePower_; }

  /**
   * @brief Generate human-readable power report
   */
  [[nodiscard]] std::string generateReport() const;

private:
  PowerConfig config_;

  // Calculated energy values (picojoules)
  double dynamicReadEnergy_ = 0.0;
  double dynamicWriteEnergy_ = 0.0;
  double prefetchEnergy_ = 0.0;

  // Calculated power values (milliwatts)
  double leakagePower_ = 0.0;

  // Accumulated statistics
  uint64_t readCount_ = 0;
  uint64_t writeCount_ = 0;
  uint64_t hitCount_ = 0;
  uint64_t missCount_ = 0;
  uint64_t prefetchCount_ = 0;

  double totalReadEnergy_ = 0.0;     // nJ
  double totalWriteEnergy_ = 0.0;    // nJ
  double totalPrefetchEnergy_ = 0.0; // nJ
  double simulationTimeNs_ = 0.0;    // ns

  // Internal calculation methods
  void calculateDynamicEnergy();
  void calculateLeakagePower();

  /**
   * @brief Calculate bitline energy component
   * @return Energy in picojoules
   */
  double calculateBitlineEnergy() const;

  /**
   * @brief Calculate wordline energy component
   * @return Energy in picojoules
   */
  double calculateWordlineEnergy() const;

  /**
   * @brief Calculate decoder energy component
   * @return Energy in picojoules
   */
  double calculateDecoderEnergy() const;

  /**
   * @brief Calculate sense amplifier energy component
   * @return Energy in picojoules
   */
  double calculateSenseAmpEnergy() const;

  /**
   * @brief Calculate output driver energy component
   * @return Energy in picojoules
   */
  double calculateOutputEnergy() const;

  /**
   * @brief Get temperature scaling factor for leakage
   * @return Multiplicative factor for leakage current
   */
  double getTemperatureFactor() const;

  /**
   * @brief Estimate total transistor count in cache
   * @return Approximate number of transistors
   */
  uint64_t estimateTransistorCount() const;
};

} // namespace cachesim

#endif // CACHESIM_POWER_MODEL_H
