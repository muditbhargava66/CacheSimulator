/**
 * @file cli_parser.h
 * @brief Command-line argument parsing for Cache Simulator
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace cachesim {

/**
 * @brief Command line options structure
 */
struct CommandLineOptions {
  std::filesystem::path configFile;
  std::filesystem::path traceFile;
  bool runBenchmark = false;
  bool visualizeResults = false;
  bool exportResults = false;
  std::filesystem::path outputPath;
  bool verbose = false;
  bool help = false;
  bool version = false;
  bool useColors = true;
  bool parallel = false;
  size_t numThreads = 0;
  bool useVictimCache = false;
  bool showCharts = false;
  bool showPowerStats = false;
  uint32_t techNode = 45; // Default 45nm
};

/**
 * @class CLIParser
 * @brief Static class for command-line parsing utilities
 */
class CLIParser {
public:
  /**
   * @brief Parse command line arguments
   * @param argc Argument count
   * @param argv Argument values
   * @return CommandLineOptions if successful, nullopt on error
   */
  static std::optional<CommandLineOptions> parse(int argc, char *argv[]);

  /**
   * @brief Print usage information
   * @param programName Name of the executable
   */
  static void printUsage(const std::string &programName);

  /**
   * @brief Print version information
   */
  static void printVersion();
};

} // namespace cachesim
