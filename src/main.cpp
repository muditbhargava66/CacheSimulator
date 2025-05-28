/**
 * @file main.cpp
 * @brief Cache Simulator main entry point
 * @author Mudit Bhargava
 * @date 2025-05-29
 * @version 1.2.0
 *
 * This file contains the main entry point for the Cache Simulator application.
 * It handles command-line argument parsing, configuration loading, and orchestrates
 * the simulation execution including benchmarking and visualization options.
 *
 * @copyright Copyright (c) 2025 Mudit Bhargava. All rights reserved.
 * @license MIT License
 */

#include <iostream>
#include <string>
#include <optional>
#include <filesystem>
#include <chrono>
#include <vector>
#include <memory>
#include <thread>
#include <algorithm>

#include "core/memory_hierarchy.h"
#include "utils/trace_parser.h"
#include "utils/trace_utils.h"
#include "utils/config_utils.h"
#include "utils/statistics.h"
#include "utils/visualization.h"
#include "utils/profiler.h"
#include "utils/logger.h"
#include "utils/benchmark.h"
#include "utils/parallel_executor.h"
#include "core/victim_cache.h"

using namespace cachesim;
namespace fs = std::filesystem;

// Command line options
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
};

// Parse command line arguments
std::optional<CommandLineOptions> parseCommandLine(int argc, char* argv[]) {
    CommandLineOptions options;
    
    // Parse arguments
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
            if (i + 1 < argc && std::isdigit(argv[i+1][0])) {
                options.numThreads = std::stoi(argv[++i]);
            }
        } else if (arg == "--victim-cache") {
            options.useVictimCache = true;
        } else if (arg == "--charts") {
            options.showCharts = true;
        } else if (arg == "-e" || arg == "--export") {
            options.exportResults = true;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                options.outputPath = argv[++i];
            } else {
                options.outputPath = "cache_sim_results.csv";
            }
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                options.configFile = argv[++i];
            } else {
                std::cerr << "Error: Missing configuration file path after " << arg << std::endl;
                return std::nullopt;
            }
        } else if (arg[0] != '-') {
            // If not a flag, treat as trace file
            options.traceFile = arg;
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            return std::nullopt;
        }
    }
    
    return options;
}

// Print usage information
void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " [options] <trace_file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help                 Show this help message and exit" << std::endl;
    std::cout << "  -v, --version              Show version information and exit" << std::endl;
    std::cout << "  -c, --config <file>        Specify configuration file" << std::endl;
    std::cout << "  -b, --benchmark            Run performance benchmark" << std::endl;
    std::cout << "  --vis, --visualize         Visualize cache behavior" << std::endl;
    std::cout << "  --no-color                 Disable colored output" << std::endl;
    std::cout << "  --verbose                  Enable verbose output" << std::endl;
    std::cout << "  -e, --export [file]        Export results to CSV file" << std::endl;
    std::cout << "  -p, --parallel [threads]   Enable parallel processing" << std::endl;
    std::cout << "  --victim-cache             Enable victim cache" << std::endl;
    std::cout << "  --charts                   Show statistical charts" << std::endl;
    std::cout << std::endl;
    std::cout << "If no configuration file is specified, the simulator uses:" << std::endl;
    std::cout << "  BLOCKSIZE=64 L1_SIZE=32KB L1_ASSOC=4 L2_SIZE=256KB L2_ASSOC=8 PREF=1 PREF_DIST=4" << std::endl;
}

/**
 * Display version information including build details
 */
void printVersion() {
    std::cout << "Cache Simulator v1.2.0" << std::endl;
    std::cout << "C++17 Edition" << std::endl;
    std::cout << "Copyright (c) 2025 Mudit Bhargava" << std::endl;
    std::cout << "Build Date: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << "Compiler: " << 
#ifdef __clang__
        "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__
#elif defined(__GNUC__)
        "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__
#elif defined(_MSC_VER)
        "MSVC " << _MSC_VER
#else
        "Unknown"
#endif
        << std::endl;
}

// Run simulation with given configuration
void runSimulation(const MemoryHierarchyConfig& config, const std::filesystem::path& traceFile,
                  bool visualize, bool exportResults, const std::filesystem::path& outputPath,
                  bool verbose) {
    // Initialize logger
    auto& logger = Logger::getInstance();
    if (verbose) {
        logger.setLogLevel(LogLevel::Debug);
    } else {
        logger.setLogLevel(LogLevel::Info);
    }
    
    if (!outputPath.empty()) {
        auto logPath = outputPath;
        logPath.replace_extension(".log");
        logger.setLogFile(logPath);
    }
    
    LOG_INFO("Starting cache simulation");
    LOG_INFO("Trace file: {}", traceFile.string());
    
    // Create memory hierarchy
    MemoryHierarchy hierarchy(config);
    
    // Initialize profiler
    MemoryProfiler profiler;
    
    // Define cache memory regions for profiling
    uint32_t l1Size = config.l1Config.size;
    profiler.defineRegion(0, l1Size - 1, "L1 Cache");
    
    if (config.l2Config) {
        uint32_t l2Size = config.l2Config->size;
        profiler.defineRegion(l1Size, l1Size + l2Size - 1, "L2 Cache");
    }
    
    // Process trace file
    LOG_INFO("Processing trace file...");
    
    TraceParser parser(traceFile);
    if (!parser.isValid()) {
        LOG_ERROR("Failed to open trace file: {}", traceFile.string());
        return;
    }
    
    // Track processing time
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process each memory access
    size_t accessCount = 0;
    std::vector<uint32_t> addresses; // For visualization
    
    while (auto access = parser.getNextAccess()) {
        hierarchy.access(access->address, access->isWrite);
        profiler.trackAccess(access->address, access->isWrite);
        
        if (visualize) {
            addresses.push_back(access->address);
        }
        
        accessCount++;
        
        if (accessCount % 100000 == 0) {
            LOG_INFO("Processed {} memory accesses", accessCount);
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    LOG_INFO("Finished processing {} memory accesses in {} ms", accessCount, duration);
    
    // Print simulation results
    std::cout << "Cache Simulation Results" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << "Trace File: " << traceFile.filename().string() << std::endl;
    std::cout << "Total Memory Accesses: " << accessCount << std::endl;
    std::cout << "Processing Time: " << duration << " ms" << std::endl;
    std::cout << "Processing Rate: " << (accessCount * 1000.0 / duration) << " accesses/sec" << std::endl;
    std::cout << std::endl;
    
    // Print memory hierarchy statistics
    hierarchy.printStats();
    
    // Print memory profiling results
    std::cout << std::endl;
    std::cout << "Memory Access Profile" << std::endl;
    std::cout << "====================" << std::endl;
    profiler.printResults();
    
    // Visualize results if requested
    if (visualize) {
        std::cout << std::endl;
        std::cout << "Memory Access Pattern Visualization" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << Visualization::visualizeAccessPattern(addresses, 80, 20, true) << std::endl;
        
        // You would need to implement a way to get actual cache state from the memory hierarchy
        // This is just a placeholder
        // std::cout << Visualization::visualizeCacheState(cache, 16, true) << std::endl;
    }
    
    // Export results if requested
    if (exportResults) {
        std::cout << std::endl;
        std::cout << "Exporting results to " << outputPath << std::endl;
        
        // Collect statistics
        Statistics stats;
        stats.addMetric("trace.file", traceFile.string());
        stats.addMetric("trace.accesses", accessCount);
        stats.addMetric("trace.processing_time_ms", duration);
        stats.addMetric("trace.processing_rate", accessCount * 1000.0 / duration);
        
        // Add memory hierarchy stats
        stats.addMetric("l1.size", config.l1Config.size);
        stats.addMetric("l1.associativity", config.l1Config.associativity);
        stats.addMetric("l1.block_size", config.l1Config.blockSize);
        stats.addMetric("l1.prefetch_enabled", config.l1Config.prefetchEnabled);
        stats.addMetric("l1.prefetch_distance", config.l1Config.prefetchDistance);
        stats.addMetric("l1.misses", hierarchy.getL1Misses());
        stats.addMetric("l1.reads", hierarchy.getL1Reads());
        stats.addMetric("l1.writes", hierarchy.getL1Writes());
        stats.addMetric("l1.miss_rate", hierarchy.getL1MissRate());
        
        if (config.l2Config) {
            stats.addMetric("l2.size", config.l2Config->size);
            stats.addMetric("l2.associativity", config.l2Config->associativity);
            stats.addMetric("l2.prefetch_enabled", config.l2Config->prefetchEnabled);
        }
        
        // Add profiler stats
        stats.addMetric("profile.pattern", static_cast<int>(profiler.detectPattern()));
        stats.addMetric("profile.unique_addresses", profiler.getUniqueAddresses());
        stats.addMetric("profile.read_write_ratio", profiler.getReadWriteRatio());
        stats.addMetric("profile.memory_footprint", profiler.getMemoryFootprint());
        
        // Export to CSV
        if (stats.exportToCsv(outputPath)) {
            std::cout << "Results exported successfully." << std::endl;
        } else {
            std::cerr << "Failed to export results." << std::endl;
        }
        
        // Also export profiler data
        auto profilerPath = outputPath;
        profilerPath.replace_extension(".profile.csv");
        if (profiler.exportToFile(profilerPath)) {
            std::cout << "Profiler data exported to " << profilerPath << std::endl;
        }
    }
}

// Run benchmark comparing different configurations
void runBenchmark(const std::vector<MemoryHierarchyConfig>& configs, const std::filesystem::path& traceFile) {
    std::cout << "Running Cache Simulator Benchmark" << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << "Trace File: " << traceFile.filename().string() << std::endl;
    std::cout << std::endl;
    
    // Ensure file exists
    if (!fs::exists(traceFile)) {
        std::cerr << "Error: Trace file " << traceFile << " does not exist." << std::endl;
        return;
    }
    
    // Create benchmark configurations
    std::vector<std::string> configNames;
    for (size_t i = 0; i < configs.size(); ++i) {
        std::ostringstream name;
        name << "Config " << (i+1);
        
        if (configs[i].l2Config) {
            name << " (L1:" << configs[i].l1Config.size / 1024 << "KB, "
                 << "L2:" << configs[i].l2Config->size / 1024 << "KB)";
        } else {
            name << " (L1:" << configs[i].l1Config.size / 1024 << "KB, No L2)";
        }
        
        configNames.push_back(name.str());
    }
    
    // Function to benchmark a single configuration
    auto benchmarkConfig = [&traceFile](const MemoryHierarchyConfig& config) {
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Parse trace
        TraceParser parser(traceFile);
        MemoryAccess access;
        
        // Process each memory access
        while (auto nextAccess = parser.getNextAccess()) {
            hierarchy.access(nextAccess->address, nextAccess->isWrite);
        }
        
        // Return miss rate as a benchmark metric
        return hierarchy.getL1MissRate();
    };
    
    // Run benchmark comparing all configurations
    std::vector<std::function<double()>> benchmarkFunctions;
    for (const auto& config : configs) {
        benchmarkFunctions.push_back([&config, &benchmarkConfig]() {
            return benchmarkConfig(config);
        });
    }
    
    // Run benchmark with 3 iterations
    Benchmark::compare("Cache Configurations",
                      3, configNames,
                      benchmarkFunctions[0], benchmarkFunctions[1],
                      benchmarkFunctions[2], benchmarkFunctions[3]);
}

int main(int argc, char* argv[]) {
    // Set up color support
    if (Visualization::supportsColors()) {
        Visualization::enableColors();
    } else {
        Visualization::disableColors();
    }
    
    // Parse command line arguments
    auto options = parseCommandLine(argc, argv);
    if (!options) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Handle help and version flags
    if (options->help) {
        printUsage(argv[0]);
        return 0;
    }
    
    if (options->version) {
        printVersion();
        return 0;
    }
    
    // Disable colors if requested
    if (!options->useColors) {
        Visualization::disableColors();
    }
    
    // Load configuration
    ConfigManager::SimulatorConfig simConfig;
    
    if (!options->configFile.empty()) {
        ConfigManager configManager(ConfigManager::ConfigFormat::JSON);
        auto loadedConfig = configManager.loadConfig(options->configFile);
        
        if (!loadedConfig) {
            std::cerr << "Error: Failed to load configuration from " << options->configFile << std::endl;
            return 1;
        }
        
        simConfig = *loadedConfig;
    } else if (argc >= 8) {
        // Legacy command line format
        ConfigManager configManager(ConfigManager::ConfigFormat::CommandLine);
        auto cmdConfig = configManager.createFromCommandLine(argc, argv);
        
        if (!cmdConfig) {
            std::cerr << "Error: Invalid command line arguments" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        
        simConfig = *cmdConfig;
    } else {
        // Use default configuration
        simConfig = ConfigManager::getDefaultConfig();
    }
    
    // Check if trace file was provided
    if (options->traceFile.empty()) {
        std::cerr << "Error: No trace file specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    if (!fs::exists(options->traceFile)) {
        std::cerr << "Error: Trace file " << options->traceFile << " does not exist" << std::endl;
        return 1;
    }
    
    // Run simulation or benchmark
    if (options->runBenchmark) {
        // Create different configurations for benchmarking
        std::vector<MemoryHierarchyConfig> benchmarkConfigs;
        
        // Base config
        benchmarkConfigs.push_back(simConfig.hierarchyConfig);
        
        // Larger L1
        auto config2 = simConfig.hierarchyConfig;
        config2.l1Config.size *= 2;
        benchmarkConfigs.push_back(config2);
        
        // Higher associativity
        auto config3 = simConfig.hierarchyConfig;
        config3.l1Config.associativity *= 2;
        benchmarkConfigs.push_back(config3);
        
        // With prefetching
        auto config4 = simConfig.hierarchyConfig;
        config4.l1Config.prefetchEnabled = true;
        config4.l1Config.prefetchDistance = 4;
        config4.useStridePrediction = true;
        config4.useAdaptivePrefetching = true;
        if (config4.l2Config) {
            config4.l2Config->prefetchEnabled = true;
            config4.l2Config->prefetchDistance = 4;
        }
        benchmarkConfigs.push_back(config4);
        
        runBenchmark(benchmarkConfigs, options->traceFile);
    } else {
        runSimulation(simConfig.hierarchyConfig, options->traceFile,
                     options->visualizeResults, options->exportResults,
                     options->outputPath, options->verbose);
    }
    
    return 0;
}