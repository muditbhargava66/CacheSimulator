/**
 * @file config_parsing_test.cpp
 * @brief Integration tests for configuration file parsing
 * @author Mudit Bhargava
 * @date 2026-01-09
 * @version 1.4.1
 *
 * Tests that all example configuration files in configs/ can be parsed
 * successfully, including those using snake_case naming convention.
 */

#include "config_utils.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class ConfigParsingTest {
public:
  static int runAllTests() {
    int passed = 0;
    int failed = 0;

    std::cout << "=== Configuration Parsing Tests ===\n\n";

    // Test 1: All JSON config files should parse successfully
    if (testAllJsonConfigs()) {
      std::cout << "[PASS] All JSON config files parsed successfully\n";
      ++passed;
    } else {
      std::cout << "[FAIL] Some JSON config files failed to parse\n";
      ++failed;
    }

    // Test 2: snake_case keys should be normalized to camelCase
    if (testSnakeCaseNormalization()) {
      std::cout << "[PASS] snake_case keys normalized correctly\n";
      ++passed;
    } else {
      std::cout << "[FAIL] snake_case key normalization failed\n";
      ++failed;
    }

    // Test 3: INI config files should parse successfully
    if (testIniConfigs()) {
      std::cout << "[PASS] INI config files parsed successfully\n";
      ++passed;
    } else {
      std::cout << "[FAIL] INI config files failed to parse\n";
      ++failed;
    }

    std::cout << "\n=== Results: " << passed << " passed, " << failed
              << " failed ===\n";

    return failed;
  }

private:
  static bool testAllJsonConfigs() {
    // Find all JSON config files
    fs::path configDir = "configs";
    if (!fs::exists(configDir)) {
      // Try from build directory
      configDir = "../configs";
    }

    if (!fs::exists(configDir)) {
      std::cerr << "  Warning: configs directory not found\n";
      return true; // Skip if not found
    }

    bool allPassed = true;
    int fileCount = 0;

    for (const auto &entry : fs::directory_iterator(configDir)) {
      if (entry.path().extension() == ".json") {
        ++fileCount;
        std::string filename = entry.path().filename().string();

        cachesim::ConfigManager manager(
            cachesim::ConfigManager::ConfigFormat::JSON);
        auto config = manager.loadConfig(entry.path());

        if (config.has_value()) {
          std::cout << "  [OK] " << filename << "\n";
        } else {
          std::cout << "  [FAILED] " << filename << "\n";
          allPassed = false;
        }
      }
    }

    std::cout << "  Tested " << fileCount << " JSON files\n";
    return allPassed && fileCount > 0;
  }

  static bool testSnakeCaseNormalization() {
    // Create a temporary config file with snake_case keys
    std::string testConfig = R"({
            "cache_hierarchy": {
                "l1": {
                    "size": 32768,
                    "associativity": 4,
                    "block_size": 64,
                    "replacement_policy": "LRU",
                    "write_policy": "WriteBack"
                }
            }
        })";

    // Write to temp file
    fs::path tempPath = fs::temp_directory_path() / "test_snake_case.json";
    std::ofstream tempFile(tempPath);
    if (!tempFile) {
      std::cerr << "  Warning: Could not create temp file\n";
      return true; // Skip if cannot create temp file
    }
    tempFile << testConfig;
    tempFile.close();

    // Try to parse
    cachesim::ConfigManager manager(
        cachesim::ConfigManager::ConfigFormat::JSON);
    auto config = manager.loadConfig(tempPath);

    // Clean up
    fs::remove(tempPath);

    if (!config.has_value()) {
      std::cerr << "  Failed to parse snake_case config\n";
      return false;
    }

    // Verify values were correctly parsed
    bool sizeCorrect = config->hierarchyConfig.l1Config.size == 32768;
    bool assocCorrect = config->hierarchyConfig.l1Config.associativity == 4;
    bool blockSizeCorrect = config->hierarchyConfig.l1Config.blockSize == 64;

    if (!sizeCorrect)
      std::cerr << "  size mismatch\n";
    if (!assocCorrect)
      std::cerr << "  associativity mismatch\n";
    if (!blockSizeCorrect)
      std::cerr << "  block_size not normalized to blockSize\n";

    return sizeCorrect && assocCorrect && blockSizeCorrect;
  }

  static bool testIniConfigs() {
    // Find INI config files if any exist
    fs::path configDir = "configs";
    if (!fs::exists(configDir)) {
      configDir = "../configs";
    }

    if (!fs::exists(configDir)) {
      return true; // Skip if not found
    }

    int fileCount = 0;
    bool allPassed = true;

    for (const auto &entry : fs::directory_iterator(configDir)) {
      if (entry.path().extension() == ".ini") {
        ++fileCount;
        std::string filename = entry.path().filename().string();

        cachesim::ConfigManager manager(
            cachesim::ConfigManager::ConfigFormat::INI);
        auto config = manager.loadConfig(entry.path());

        if (config.has_value()) {
          std::cout << "  [OK] " << filename << "\n";
        } else {
          std::cout << "  [FAILED] " << filename << "\n";
          allPassed = false;
        }
      }
    }

    if (fileCount == 0) {
      std::cout << "  No INI files found (skipped)\n";
    } else {
      std::cout << "  Tested " << fileCount << " INI files\n";
    }

    return allPassed;
  }
};

int main() { return ConfigParsingTest::runAllTests(); }
