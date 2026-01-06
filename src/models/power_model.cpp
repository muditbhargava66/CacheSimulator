/**
 * @file power_model.cpp
 * @brief Implementation of CACTI-inspired power model
 * @author Mudit Bhargava
 * @date 2026-01-06
 * @version 1.3.0
 */

#include "power_model.h"
#include "power_constants.h"
#include <algorithm>
#include <iomanip>
#include <sstream>


namespace cachesim {

PowerModel::PowerModel(const PowerConfig &config) : config_(config) {
  if (config_.enabled) {
    calculateDynamicEnergy();
    calculateLeakagePower();
  }
}

PowerModel::PowerModel() : config_() { config_.enabled = false; }

void PowerModel::configure(const PowerConfig &config) {
  config_ = config;
  if (config_.enabled) {
    calculateDynamicEnergy();
    calculateLeakagePower();
  }
  reset();
}

void PowerModel::recordAccess(bool isWrite, bool isHit) {
  if (!config_.enabled)
    return;

  if (isWrite) {
    writeCount_++;
    totalWriteEnergy_ += dynamicWriteEnergy_ / 1000.0; // pJ to nJ
  } else {
    readCount_++;
    totalReadEnergy_ += dynamicReadEnergy_ / 1000.0; // pJ to nJ
  }

  if (isHit) {
    hitCount_++;
  } else {
    missCount_++;
  }
}

void PowerModel::recordPrefetch() {
  if (!config_.enabled)
    return;

  prefetchCount_++;
  totalPrefetchEnergy_ += prefetchEnergy_ / 1000.0; // pJ to nJ
}

void PowerModel::updateSimulationTime(double elapsedNs) {
  simulationTimeNs_ = elapsedNs;
}

PowerStats PowerModel::getStats() const {
  PowerStats stats;

  // Per-access energy
  stats.dynamicReadEnergy = dynamicReadEnergy_;
  stats.dynamicWriteEnergy = dynamicWriteEnergy_;

  // Power
  stats.leakagePower = leakagePower_;

  // Accumulated energy
  stats.totalReadEnergy = totalReadEnergy_;
  stats.totalWriteEnergy = totalWriteEnergy_;
  stats.totalDynamicEnergy =
      totalReadEnergy_ + totalWriteEnergy_ + totalPrefetchEnergy_;

  // Leakage energy = leakage power × time
  double simTimeSeconds = simulationTimeNs_ * 1e-9;
  stats.totalLeakageEnergy =
      leakagePower_ * simTimeSeconds * 1e6; // mW × s = mJ → nJ

  stats.totalEnergy = stats.totalDynamicEnergy + stats.totalLeakageEnergy;

  // Access counts
  stats.readCount = readCount_;
  stats.writeCount = writeCount_;
  stats.hitCount = hitCount_;
  stats.missCount = missCount_;

  // Derived metrics
  uint64_t totalAccesses = readCount_ + writeCount_;
  if (totalAccesses > 0) {
    stats.energyPerAccess =
        (stats.totalDynamicEnergy * 1000.0) / totalAccesses; // nJ to pJ
  }

  // Average dynamic power
  if (simulationTimeNs_ > 0) {
    stats.averageDynamicPower = (stats.totalDynamicEnergy * 1e-9) /
                                (simulationTimeNs_ * 1e-9) *
                                1000.0; // nJ/ns = W → mW
    stats.totalPower = stats.averageDynamicPower + stats.leakagePower;
  } else {
    stats.totalPower = stats.leakagePower;
  }

  // Energy-Delay Product (EDP)
  // Access time approximation based on technology and cache size
  double accessTimeNs = 1.0 / (config_.frequency * 1e-9); // One cycle in ns
  stats.energyDelayProduct = stats.energyPerAccess * accessTimeNs;

  stats.simulationTimeNs = simulationTimeNs_;

  return stats;
}

void PowerModel::reset() {
  readCount_ = 0;
  writeCount_ = 0;
  hitCount_ = 0;
  missCount_ = 0;
  prefetchCount_ = 0;
  totalReadEnergy_ = 0.0;
  totalWriteEnergy_ = 0.0;
  totalPrefetchEnergy_ = 0.0;
  simulationTimeNs_ = 0.0;
}

void PowerModel::calculateDynamicEnergy() {
  // Get technology parameters
  const auto &tech = power::getTechParams(config_.techNode);

  // Calculate energy components
  double bitlineEnergy = calculateBitlineEnergy();
  double wordlineEnergy = calculateWordlineEnergy();
  double decoderEnergy = calculateDecoderEnergy();
  double senseAmpEnergy = calculateSenseAmpEnergy();
  double outputEnergy = calculateOutputEnergy();

  // Total read energy (picojoules)
  dynamicReadEnergy_ = bitlineEnergy + wordlineEnergy + decoderEnergy +
                       senseAmpEnergy + outputEnergy;

  // Write energy is higher due to driving bitlines both ways
  dynamicWriteEnergy_ =
      dynamicReadEnergy_ * power::PowerConstants::WRITE_ENERGY_FACTOR;

  // Prefetch energy (slightly lower due to relaxed timing)
  prefetchEnergy_ =
      dynamicReadEnergy_ * power::PowerConstants::PREFETCH_ENERGY_FACTOR;
}

void PowerModel::calculateLeakagePower() {
  const auto &tech = power::getTechParams(config_.techNode);

  // Estimate transistor count
  uint64_t transistorCount = estimateTransistorCount();

  // Temperature scaling factor
  double tempFactor = getTemperatureFactor();

  // Subthreshold leakage (dominant in modern processes)
  // P_sub = I_sub × Vdd × N_transistors × temp_factor
  double subthresholdPower = tech.subthresholdI * config_.vdd *
                             transistorCount * tempFactor *
                             1e3; // Convert to mW

  // Gate leakage (significant in ultra-thin oxide)
  double gatePower =
      tech.gateLeakageI * config_.vdd * transistorCount * 1e3; // Convert to mW

  // Total leakage power (milliwatts)
  leakagePower_ = subthresholdPower + gatePower;
}

double PowerModel::calculateBitlineEnergy() const {
  const auto &tech = power::getTechParams(config_.techNode);

  // Bitline energy = C_bl × Vdd² × num_bitlines_read
  // num_bitlines = block_size × 8 bits × 2 (differential)
  uint32_t numBitlines = config_.blockSize * 8 * 2;

  // Energy in femtojoules, convert to picojoules
  double energy =
      tech.bitlineCap * 1e-15 * config_.vdd * config_.vdd * numBitlines;
  return energy * 1e12; // fJ to pJ
}

double PowerModel::calculateWordlineEnergy() const {
  const auto &tech = power::getTechParams(config_.techNode);

  // Wordline energy = C_wl × Vdd² × wordline_length
  // Wordline spans associativity × block_size columns
  uint32_t wordlineLength = config_.associativity * config_.blockSize * 8;

  double energy =
      tech.wordlineCap * 1e-15 * config_.vdd * config_.vdd * wordlineLength;
  return energy * 1e12; // fJ to pJ
}

double PowerModel::calculateDecoderEnergy() const {
  const auto &tech = power::getTechParams(config_.techNode);

  // Decoder energy scales with log2(sets) × associativity
  uint32_t numSets =
      config_.cacheSize / (config_.blockSize * config_.associativity);
  uint32_t indexBits = 0;
  uint32_t temp = numSets;
  while (temp > 1) {
    indexBits++;
    temp >>= 1;
  }

  double energy = tech.decoderEnergyPerBit * indexBits * config_.associativity;
  return energy; // Already in pJ
}

double PowerModel::calculateSenseAmpEnergy() const {
  const auto &tech = power::getTechParams(config_.techNode);

  // One sense amp per bitline pair × block size
  uint32_t numSenseAmps = config_.blockSize * 8;

  double energy = tech.senseAmpEnergy * numSenseAmps;
  return energy; // Already in pJ (from fJ × 1000)
}

double PowerModel::calculateOutputEnergy() const {
  // Output driver energy ~ 10% of total dynamic energy
  return (calculateBitlineEnergy() + calculateWordlineEnergy()) * 0.1;
}

double PowerModel::getTemperatureFactor() const {
  // Exponential increase in leakage with temperature
  // Factor = exp((T - T_ref) / T_coefficient)
  double tempDiff = config_.temperature - power::PowerConstants::REFERENCE_TEMP;
  return std::exp(tempDiff / power::PowerConstants::TEMP_COEFFICIENT);
}

uint64_t PowerModel::estimateTransistorCount() const {
  // 6T SRAM cell per bit
  uint64_t dataBits = static_cast<uint64_t>(config_.cacheSize) * 8;
  uint64_t dataTransistors =
      dataBits * power::PowerConstants::TRANSISTORS_PER_SRAM_CELL;

  // Tag storage
  uint32_t numSets =
      config_.cacheSize / (config_.blockSize * config_.associativity);
  uint64_t tagBits =
      static_cast<uint64_t>(config_.tagBits) * numSets * config_.associativity;
  uint64_t tagTransistors =
      tagBits * power::PowerConstants::TRANSISTORS_PER_SRAM_CELL;

  // Peripheral circuits (~30% overhead)
  uint64_t peripheralTransistors =
      static_cast<uint64_t>((dataTransistors + tagTransistors) * 0.3);

  return dataTransistors + tagTransistors + peripheralTransistors;
}

std::string PowerModel::generateReport() const {
  std::ostringstream oss;
  auto stats = getStats();

  oss << std::fixed << std::setprecision(3);

  oss << "\n╔═══════════════════════════════════════════════════════════╗\n";
  oss << "║              Power and Energy Analysis                     ║\n";
  oss << "╠═══════════════════════════════════════════════════════════╣\n";

  oss << "║ Technology Node:        " << std::setw(8) << config_.techNode
      << " nm" << std::setw(22) << "║\n";
  oss << "║ Supply Voltage:         " << std::setw(8) << config_.vdd << " V"
      << std::setw(23) << "║\n";
  oss << "║ Temperature:            " << std::setw(8) << config_.temperature
      << " K" << std::setw(23) << "║\n";

  oss << "╠═══════════════════════════════════════════════════════════╣\n";
  oss << "║ Per-Access Energy                                          ║\n";
  oss << "╠═══════════════════════════════════════════════════════════╣\n";

  oss << "║ Dynamic Read Energy:    " << std::setw(8) << stats.dynamicReadEnergy
      << " pJ" << std::setw(22) << "║\n";
  oss << "║ Dynamic Write Energy:   " << std::setw(8)
      << stats.dynamicWriteEnergy << " pJ" << std::setw(22) << "║\n";
  oss << "║ Average Energy/Access:  " << std::setw(8) << stats.energyPerAccess
      << " pJ" << std::setw(22) << "║\n";

  oss << "╠═══════════════════════════════════════════════════════════╣\n";
  oss << "║ Power Breakdown                                            ║\n";
  oss << "╠═══════════════════════════════════════════════════════════╣\n";

  oss << "║ Static (Leakage) Power: " << std::setw(8) << stats.leakagePower
      << " mW" << std::setw(22) << "║\n";
  oss << "║ Avg Dynamic Power:      " << std::setw(8)
      << stats.averageDynamicPower << " mW" << std::setw(22) << "║\n";
  oss << "║ Total Power:            " << std::setw(8) << stats.totalPower
      << " mW" << std::setw(22) << "║\n";

  oss << "╠═══════════════════════════════════════════════════════════╣\n";
  oss << "║ Total Energy                                               ║\n";
  oss << "╠═══════════════════════════════════════════════════════════╣\n";

  oss << "║ Read Energy:            " << std::setw(8) << stats.totalReadEnergy
      << " nJ" << std::setw(22) << "║\n";
  oss << "║ Write Energy:           " << std::setw(8) << stats.totalWriteEnergy
      << " nJ" << std::setw(22) << "║\n";
  oss << "║ Dynamic Energy:         " << std::setw(8)
      << stats.totalDynamicEnergy << " nJ" << std::setw(22) << "║\n";
  oss << "║ Leakage Energy:         " << std::setw(8)
      << stats.totalLeakageEnergy << " nJ" << std::setw(22) << "║\n";
  oss << "║ Total Energy:           " << std::setw(8) << stats.totalEnergy
      << " nJ" << std::setw(22) << "║\n";

  oss << "╠═══════════════════════════════════════════════════════════╣\n";
  oss << "║ Efficiency Metrics                                         ║\n";
  oss << "╠═══════════════════════════════════════════════════════════╣\n";

  oss << "║ Energy-Delay Product:   " << std::setw(8)
      << stats.energyDelayProduct << " pJ·ns" << std::setw(19) << "║\n";

  oss << "╚═══════════════════════════════════════════════════════════╝\n";

  return oss.str();
}

} // namespace cachesim
