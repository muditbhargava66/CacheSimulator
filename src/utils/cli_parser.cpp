/**
 * @file cli_parser.cpp
 * @brief Command-line argument parsing implementation
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include "cli_parser.h"
#include <cctype>
#include <iostream>

namespace cachesim {

std::optional<CommandLineOptions> CLIParser::parse(int argc, char *argv[]) {
  CommandLineOptions options;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      options.help = true;
    } else if (arg == "-v" || arg == "--version") {
      options.version = true;
    } else if (arg == "-b" || arg == "--benchmark") {
      options.runBenchmark = true;
    } else if (arg == "--vis" || arg == "--visualize") {
      options.visualizeResults = true;
    } else if (arg == "--no-color") {
      options.useColors = false;
    } else if (arg == "--verbose") {
      options.verbose = true;
    } else if (arg == "-p" || arg == "--parallel") {
      options.parallel = true;
      if (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
        options.numThreads = std::stoi(argv[++i]);
      }
    } else if (arg == "--victim-cache") {
      options.useVictimCache = true;
    } else if (arg == "--charts") {
      options.showCharts = true;
    } else if (arg == "--power") {
      options.showPowerStats = true;
    } else if (arg == "--tech-node") {
      if (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
        options.techNode = std::stoi(argv[++i]);
      }
    } else if (arg == "-e" || arg == "--export") {
      options.exportResults = true;
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        options.outputPath = argv[++i];
      } else {
        options.outputPath = "cache_sim_results.csv";
      }
    } else if (arg == "-c" || arg == "--config") {
      if (i + 1 < argc) {
        options.configFile = argv[++i];
      } else {
        std::cerr << "Error: Missing configuration file path after " << arg
                  << std::endl;
        return std::nullopt;
      }
    } else if (arg[0] != '-') {
      options.traceFile = arg;
    } else {
      std::cerr << "Error: Unknown option: " << arg << std::endl;
      return std::nullopt;
    }
  }

  return options;
}

void CLIParser::printUsage(const std::string &programName) {
  std::cout << "Usage: " << programName << " [options] <trace_file>"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h, --help                 Show this help message and exit"
            << std::endl;
  std::cout << "  -v, --version              Show version information and exit"
            << std::endl;
  std::cout << "  -c, --config <file>        Specify configuration file"
            << std::endl;
  std::cout << "  -b, --benchmark            Run performance benchmark"
            << std::endl;
  std::cout << "  --vis, --visualize         Visualize cache behavior"
            << std::endl;
  std::cout << "  --no-color                 Disable colored output"
            << std::endl;
  std::cout << "  --verbose                  Enable verbose output"
            << std::endl;
  std::cout << "  -e, --export [file]        Export results to CSV file"
            << std::endl;
  std::cout << "  -p, --parallel [threads]   Enable parallel processing"
            << std::endl;
  std::cout << "  --victim-cache             Enable victim cache" << std::endl;
  std::cout << "  --charts                   Show statistical charts"
            << std::endl;
  std::cout << "  --power                    Show power and energy analysis"
            << std::endl;
  std::cout << "  --tech-node <nm>           Technology node (7,14,22,32,45) "
               "default:45"
            << std::endl;
  std::cout << std::endl;
  std::cout << "If no configuration file is specified, the simulator uses:"
            << std::endl;
  std::cout << "  BLOCKSIZE=64 L1_SIZE=32KB L1_ASSOC=4 L2_SIZE=256KB "
               "L2_ASSOC=8 PREF=1 PREF_DIST=4"
            << std::endl;
}

void CLIParser::printVersion() {
  std::cout << "Cache Simulator v1.4.3" << std::endl;
  std::cout << "C++20 Edition" << std::endl;
  std::cout << "Copyright (c) 2025-2026 Mudit Bhargava" << std::endl;
  std::cout << "Build Date: " << __DATE__ << " " << __TIME__ << std::endl;
  std::cout << "Compiler: " <<
#ifdef __clang__
      "Clang " << __clang_major__ << "." << __clang_minor__ << "."
            << __clang_patchlevel__
#elif defined(__GNUC__)
      "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__
#elif defined(_MSC_VER)
      "MSVC " << _MSC_VER
#else
      "Unknown"
#endif
            << std::endl;
}

} // namespace cachesim
