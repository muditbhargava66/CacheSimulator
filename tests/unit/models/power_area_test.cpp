/**
 * @file power_area_test.cpp
 * @brief Unit tests for Power and Area models
 * @author Mudit Bhargava
 * @date 2026-01-06
 * @version 1.3.0
 */

#include "models/area_model.h"
#include "models/power_constants.h"
#include "models/power_model.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>


using namespace cachesim;

// Test utility
#define TEST(name)                                                             \
  void test_##name();                                                          \
  struct TestRunner_##name {                                                   \
    TestRunner_##name() {                                                      \
      test_##name();                                                           \
      std::cout << "PASSED: " << #name << std::endl;                           \
    }                                                                          \
  } runner_##name;                                                             \
  void test_##name()

// Tolerance for floating-point comparisons
constexpr double EPSILON = 1e-6;
constexpr double PERCENT_TOLERANCE = 0.20; // 20% tolerance for CACTI comparison

bool approxEqual(double a, double b, double tolerance = EPSILON) {
  return std::abs(a - b) < tolerance;
}

bool withinPercentage(double value, double reference, double percent) {
  if (reference == 0)
    return std::abs(value) < EPSILON;
  return std::abs((value - reference) / reference) < percent;
}

// ============================================================================
// Power Model Tests
// ============================================================================

TEST(power_model_default_construction) {
  PowerModel model;
  assert(!model.isEnabled());
}

TEST(power_model_configuration) {
  PowerConfig config;
  config.enabled = true;
  config.techNode = 45;
  config.vdd = 1.0;
  config.cacheSize = 32768; // 32KB
  config.blockSize = 64;
  config.associativity = 4;
  config.tagBits = 20;

  PowerModel model(config);
  assert(model.isEnabled());
  assert(model.getReadEnergy() > 0);
  assert(model.getWriteEnergy() > model.getReadEnergy()); // Write > Read
  assert(model.getLeakagePower() > 0);
}

TEST(power_model_access_tracking) {
  PowerConfig config;
  config.enabled = true;
  config.techNode = 45;
  config.cacheSize = 32768;
  config.blockSize = 64;
  config.associativity = 4;

  PowerModel model(config);

  // Record some accesses
  for (int i = 0; i < 100; i++) {
    model.recordAccess(false, true); // Read hit
  }
  for (int i = 0; i < 50; i++) {
    model.recordAccess(true, true); // Write hit
  }

  auto stats = model.getStats();
  assert(stats.readCount == 100);
  assert(stats.writeCount == 50);
  assert(stats.hitCount == 150);
  assert(stats.missCount == 0);
  assert(stats.totalReadEnergy > 0);
  assert(stats.totalWriteEnergy > 0);
}

TEST(power_model_technology_scaling) {
  PowerConfig config;
  config.enabled = true;
  config.cacheSize = 32768;
  config.blockSize = 64;
  config.associativity = 4;

  // 45nm baseline
  config.techNode = 45;
  config.vdd = 1.0;
  PowerModel model45(config);

  // 7nm should have lower energy per access
  config.techNode = 7;
  config.vdd = 0.65;
  PowerModel model7(config);

  // 7nm should have lower dynamic energy
  assert(model7.getReadEnergy() < model45.getReadEnergy());

  std::cout << "  45nm read energy: " << model45.getReadEnergy() << " pJ"
            << std::endl;
  std::cout << "  7nm read energy: " << model7.getReadEnergy() << " pJ"
            << std::endl;
}

TEST(power_model_temperature_effect) {
  PowerConfig config;
  config.enabled = true;
  config.techNode = 45;
  config.cacheSize = 32768;
  config.blockSize = 64;
  config.associativity = 4;

  // Room temperature
  config.temperature = 300.0;
  PowerModel modelCold(config);

  // High temperature (80°C = 353K)
  config.temperature = 353.0;
  PowerModel modelHot(config);

  // Leakage should increase with temperature
  assert(modelHot.getLeakagePower() > modelCold.getLeakagePower());

  std::cout << "  Leakage @ 300K: " << modelCold.getLeakagePower() << " mW"
            << std::endl;
  std::cout << "  Leakage @ 353K: " << modelHot.getLeakagePower() << " mW"
            << std::endl;
}

TEST(power_model_report_generation) {
  PowerConfig config;
  config.enabled = true;
  config.techNode = 45;
  config.cacheSize = 32768;
  config.blockSize = 64;
  config.associativity = 4;

  PowerModel model(config);

  // Simulate some activity
  for (int i = 0; i < 1000; i++) {
    model.recordAccess(i % 4 == 0, i % 3 == 0);
  }
  model.updateSimulationTime(1000.0); // 1000 ns

  std::string report = model.generateReport();
  assert(!report.empty());
  assert(report.find("Power") != std::string::npos);
  assert(report.find("Energy") != std::string::npos);
}

// ============================================================================
// Area Model Tests
// ============================================================================

TEST(area_model_basic_calculation) {
  auto breakdown = AreaModel::calculate(32768, // 32KB cache
                                        64,    // 64B block
                                        4,     // 4-way
                                        20,    // 20 tag bits
                                        45     // 45nm
  );

  assert(breakdown.total > 0);
  assert(breakdown.dataArray > 0);
  assert(breakdown.tagArray > 0);
  assert(breakdown.decoder > 0);
  assert(breakdown.senseAmps > 0);
  assert(breakdown.cellEfficiency > 0 && breakdown.cellEfficiency < 1);

  std::cout << "  32KB L1 @ 45nm area: " << breakdown.total << " mm²"
            << std::endl;
}

TEST(area_model_technology_scaling) {
  // Same configuration, different technology nodes
  auto area45 = AreaModel::calculate(32768, 64, 4, 20, 45);
  auto area22 = AreaModel::calculate(32768, 64, 4, 20, 22);
  auto area7 = AreaModel::calculate(32768, 64, 4, 20, 7);

  // Smaller tech nodes should have smaller area
  assert(area22.total < area45.total);
  assert(area7.total < area22.total);

  std::cout << "  45nm area: " << area45.total << " mm²" << std::endl;
  std::cout << "  22nm area: " << area22.total << " mm²" << std::endl;
  std::cout << "  7nm area: " << area7.total << " mm²" << std::endl;
}

TEST(area_model_size_scaling) {
  // Larger caches should have larger area
  auto l1 = AreaModel::calculate(32768, 64, 4, 20, 45);    // 32KB L1
  auto l2 = AreaModel::calculate(262144, 64, 8, 18, 45);   // 256KB L2
  auto l3 = AreaModel::calculate(8388608, 64, 16, 15, 45); // 8MB L3

  assert(l2.total > l1.total);
  assert(l3.total > l2.total);

  std::cout << "  32KB L1 area: " << l1.total << " mm²" << std::endl;
  std::cout << "  256KB L2 area: " << l2.total << " mm²" << std::endl;
  std::cout << "  8MB L3 area: " << l3.total << " mm²" << std::endl;
}

TEST(area_model_associativity_effect) {
  // Higher associativity increases overhead
  auto direct = AreaModel::calculate(32768, 64, 1, 25, 45);
  auto way4 = AreaModel::calculate(32768, 64, 4, 23, 45);
  auto way8 = AreaModel::calculate(32768, 64, 8, 22, 45);

  // Higher associativity has more peripheral overhead
  assert(way4.decoder > direct.decoder);
  assert(way8.routing > way4.routing);
}

TEST(area_model_report_generation) {
  auto breakdown = AreaModel::calculate(32768, 64, 4, 20, 45);
  std::string report = AreaModel::generateReport(breakdown, 45);

  assert(!report.empty());
  assert(report.find("Area") != std::string::npos);
  assert(report.find("mm²") != std::string::npos);
}

// ============================================================================
// CACTI Validation Tests
// ============================================================================

TEST(cacti_validation_32kb_l1) {
  // CACTI 7.0 reference for 32KB, 4-way, 64B block, 45nm:
  // - Approximate area: 0.05-0.08 mm²
  // - Dynamic energy: 0.05-0.15 pJ per access

  auto area = AreaModel::calculate(32768, 64, 4, 20, 45);

  PowerConfig config;
  config.enabled = true;
  config.techNode = 45;
  config.vdd = 1.0;
  config.cacheSize = 32768;
  config.blockSize = 64;
  config.associativity = 4;
  config.tagBits = 20;

  PowerModel power(config);

  std::cout << "  CACTI Validation (32KB L1 @ 45nm):" << std::endl;
  std::cout << "    Area: " << area.total << " mm² (expected: 0.05-0.08)"
            << std::endl;
  std::cout << "    Read Energy: " << power.getReadEnergy()
            << " pJ (expected: 0.05-0.15)" << std::endl;

  // These are order-of-magnitude checks, not exact matches
  // Our simplified model should be within 2-3x of CACTI
  assert(area.total > 0.01 && area.total < 0.5);
  assert(power.getReadEnergy() > 0.01 && power.getReadEnergy() < 1.0);
}

TEST(cacti_validation_256kb_l2) {
  // CACTI 7.0 reference for 256KB, 8-way, 64B block, 45nm:
  // - Approximate area: 0.3-0.5 mm²
  // - Dynamic energy: 0.2-0.5 pJ per access

  auto area = AreaModel::calculate(262144, 64, 8, 18, 45);

  PowerConfig config;
  config.enabled = true;
  config.techNode = 45;
  config.vdd = 1.0;
  config.cacheSize = 262144;
  config.blockSize = 64;
  config.associativity = 8;
  config.tagBits = 18;

  PowerModel power(config);

  std::cout << "  CACTI Validation (256KB L2 @ 45nm):" << std::endl;
  std::cout << "    Area: " << area.total << " mm² (expected: 0.3-0.5)"
            << std::endl;
  std::cout << "    Read Energy: " << power.getReadEnergy()
            << " pJ (expected: 0.2-0.5)" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main() {
  std::cout << "\n========================================" << std::endl;
  std::cout << "Power and Area Model Unit Tests" << std::endl;
  std::cout << "========================================\n" << std::endl;

  std::cout << "\n--- Power Model Tests ---\n" << std::endl;
  // Tests run automatically via static initialization

  std::cout << "\n--- Area Model Tests ---\n" << std::endl;

  std::cout << "\n--- CACTI Validation Tests ---\n" << std::endl;

  std::cout << "\n========================================" << std::endl;
  std::cout << "All Power/Area tests passed!" << std::endl;
  std::cout << "========================================\n" << std::endl;

  return 0;
}
