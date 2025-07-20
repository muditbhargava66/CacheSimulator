/**
 * @file performance_comparison.cpp
 * @brief Tool to compare performance of different cache configurations
 * @author Mudit Bhargava
 * @date 2025-06-01
 * @version 1.2.0
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <thread>
#include <future>
#include "../src/core/memory_hierarchy.h"
#include "../src/core/victim_cache.h"
#include "../src/utils/trace_parser.h"
#include "../src/utils/visualization.h"
#include "../src/utils/parallel_executor.h"
#include "../src/utils/config_utils.h"

using namespace cachesim;
using namespace std::chrono;

struct SimulationResult {
    std::string configName;
    MemoryHierarchyConfig config;
    
    // Performance metrics
    double l1HitRate;
    double l2HitRate;
    double overallHitRate;
    double avgAccessTime;
    
    // Statistics
    uint64_t totalAccesses;
    uint64_t l1Hits;
    uint64_t l1Misses;
    uint64_t l2Hits;
    uint64_t l2Misses;
    uint64_t writebacks;
    uint64_t prefetches;
    
    // Victim cache stats (if used)
    double victimCacheHitRate;
    uint64_t victimCacheHits;
    
    // Timing
    double simulationTimeMs;
    double accessesPerSecond;
};

class PerformanceComparison {
public:
    void addConfiguration(const std::string& name, const MemoryHierarchyConfig& config) {
        configurations_.push_back({name, config});
    }
    
    void addDefaultConfigurations() {
        // Basic L1-only configuration
        {
            MemoryHierarchyConfig config;
            config.l1Config = CacheConfig(32768, 4, 64);  // 32KB, 4-way
            addConfiguration("Basic L1 (32KB, 4-way)", config);
        }
        
        // L1+L2 configuration
        {
            MemoryHierarchyConfig config;
            config.l1Config = CacheConfig(32768, 4, 64);
            config.l2Config = CacheConfig(262144, 8, 64);  // 256KB, 8-way
            addConfiguration("L1+L2 (32KB+256KB)", config);
        }
        
        // With prefetching
        {
            MemoryHierarchyConfig config;
            config.l1Config = CacheConfig(32768, 4, 64, true, 4);
            config.l2Config = CacheConfig(262144, 8, 64, true, 8);
            config.useStridePrediction = true;
            addConfiguration("L1+L2 with Prefetching", config);
        }
        
        // v1.2.0 NRU policy
        {
            MemoryHierarchyConfig config;
            config.l1Config = CacheConfig(32768, 4, 64);
            config.l1Config.replacementPolicy = ReplacementPolicy::NRU;
            config.l2Config = CacheConfig(262144, 8, 64);
            config.l2Config->replacementPolicy = ReplacementPolicy::NRU;
            addConfiguration("NRU Replacement Policy", config);
        }
        
        // v1.2.0 Write-through no-allocate
        {
            MemoryHierarchyConfig config;
            config.l1Config = CacheConfig(32768, 4, 64);
            config.l1Config.writePolicy = WritePolicy::WriteThrough;
            config.l2Config = CacheConfig(262144, 8, 64);
            addConfiguration("Write-Through No-Allocate", config);
        }
        
        // v1.2.0 With victim cache
        {
            MemoryHierarchyConfig config;
            config.l1Config = CacheConfig(16384, 2, 64);  // Smaller L1
            config.l2Config = CacheConfig(262144, 8, 64);
            addConfiguration("Small L1 + Victim Cache", config);
        }
        
        // High-performance configuration
        {
            MemoryHierarchyConfig config;
            config.l1Config = CacheConfig(65536, 8, 64, true, 8);  // 64KB, 8-way
            config.l1Config.replacementPolicy = ReplacementPolicy::LRU;
            config.l2Config = CacheConfig(1048576, 16, 64, true, 16);  // 1MB, 16-way
            config.l2Config->replacementPolicy = ReplacementPolicy::PLRU;
            config.useAdaptivePrefetching = true;
            addConfiguration("High-Performance", config);
        }
    }
    
    std::vector<SimulationResult> runComparisons(const std::string& traceFile, 
                                                bool parallel = true) {
        std::cout << "Running performance comparisons on: " << traceFile << std::endl;
        std::cout << "Number of configurations: " << configurations_.size() << std::endl;
        std::cout << "Parallel execution: " << (parallel ? "Yes" : "No") << std::endl;
        std::cout << std::endl;
        
        std::vector<SimulationResult> results;
        
        if (parallel) {
            // Run simulations in parallel
            ThreadPool pool(std::thread::hardware_concurrency());
            std::vector<std::future<SimulationResult>> futures;
            
            for (const auto& [name, config] : configurations_) {
                futures.push_back(
                    pool.enqueue([this, name, config, traceFile]() {
                        return runSingleSimulation(name, config, traceFile);
                    })
                );
            }
            
            // Collect results
            for (auto& future : futures) {
                results.push_back(future.get());
                printProgress(results.size(), configurations_.size());
            }
        } else {
            // Run sequentially
            for (const auto& [name, config] : configurations_) {
                results.push_back(runSingleSimulation(name, config, traceFile));
                printProgress(results.size(), configurations_.size());
            }
        }
        
        std::cout << std::endl;
        return results;
    }
    
    void printComparisonTable(const std::vector<SimulationResult>& results) {
        std::cout << "\n=== Performance Comparison Results ===" << std::endl;
        std::cout << std::string(100, '-') << std::endl;
        
        // Header
        std::cout << std::left << std::setw(25) << "Configuration"
                  << std::right << std::setw(10) << "L1 Hit%"
                  << std::setw(10) << "L2 Hit%"
                  << std::setw(12) << "Overall%"
                  << std::setw(12) << "Avg Time"
                  << std::setw(12) << "Sim Time"
                  << std::setw(15) << "Accesses/s" << std::endl;
        std::cout << std::string(100, '-') << std::endl;
        
        // Sort by overall hit rate
        auto sorted = results;
        std::sort(sorted.begin(), sorted.end(), 
            [](const auto& a, const auto& b) {
                return a.overallHitRate > b.overallHitRate;
            });
        
        // Print results
        for (const auto& result : sorted) {
            std::cout << std::left << std::setw(25) << result.configName
                      << std::right << std::fixed << std::setprecision(1)
                      << std::setw(10) << (result.l1HitRate * 100)
                      << std::setw(10) << (result.l2HitRate * 100)
                      << std::setw(12) << (result.overallHitRate * 100)
                      << std::setw(12) << result.avgAccessTime
                      << std::setw(12) << result.simulationTimeMs
                      << std::setw(15) << std::scientific << std::setprecision(2)
                      << result.accessesPerSecond << std::endl;
        }
        
        std::cout << std::string(100, '-') << std::endl;
    }
    
    void generateCharts(const std::vector<SimulationResult>& results) {
        // Hit rate comparison
        std::cout << "\nHit Rate Comparison:" << std::endl;
        std::vector<std::pair<std::string, double>> hitRates;
        for (const auto& result : results) {
            hitRates.push_back({result.configName, result.overallHitRate * 100});
        }
        std::cout << Visualization::generateHistogram(hitRates, 50);
        
        // Average access time
        std::cout << "\nAverage Access Time:" << std::endl;
        std::vector<std::pair<std::string, double>> accessTimes;
        for (const auto& result : results) {
            accessTimes.push_back({result.configName, result.avgAccessTime});
        }
        std::cout << Visualization::generateHistogram(accessTimes, 50);
        
        // Simulation performance
        std::cout << "\nSimulation Performance (accesses/sec):" << std::endl;
        std::vector<std::pair<std::string, double>> perfData;
        for (const auto& result : results) {
            perfData.push_back({result.configName, result.accessesPerSecond / 1e6});  // Millions
        }
        std::cout << Visualization::generateHistogram(perfData, 50);
    }
    
    void exportResults(const std::vector<SimulationResult>& results, 
                      const std::string& filename) {
        std::ofstream out(filename);
        
        // Header
        out << "Configuration,L1_Size,L1_Assoc,L2_Size,L2_Assoc,"
            << "L1_Hit_Rate,L2_Hit_Rate,Overall_Hit_Rate,"
            << "Avg_Access_Time,Writebacks,Prefetches,"
            << "Victim_Cache_Hits,Simulation_Time_ms,Accesses_per_sec\n";
        
        // Data rows
        for (const auto& result : results) {
            out << result.configName << ","
                << result.config.l1Config.size << ","
                << result.config.l1Config.associativity << ","
                << (result.config.l2Config ? result.config.l2Config->size : 0) << ","
                << (result.config.l2Config ? result.config.l2Config->associativity : 0) << ","
                << result.l1HitRate << ","
                << result.l2HitRate << ","
                << result.overallHitRate << ","
                << result.avgAccessTime << ","
                << result.writebacks << ","
                << result.prefetches << ","
                << result.victimCacheHits << ","
                << result.simulationTimeMs << ","
                << result.accessesPerSecond << "\n";
        }
        
        std::cout << "\nResults exported to: " << filename << std::endl;
    }
    
    void generateRecommendations(const std::vector<SimulationResult>& results) {
        std::cout << "\n=== Recommendations ===" << std::endl;
        
        // Find best configurations
        auto bestHitRate = std::max_element(results.begin(), results.end(),
            [](const auto& a, const auto& b) {
                return a.overallHitRate < b.overallHitRate;
            });
        
        auto bestSpeed = std::max_element(results.begin(), results.end(),
            [](const auto& a, const auto& b) {
                return a.accessesPerSecond < b.accessesPerSecond;
            });
        
        auto bestAvgTime = std::min_element(results.begin(), results.end(),
            [](const auto& a, const auto& b) {
                return a.avgAccessTime > b.avgAccessTime;
            });
        
        std::cout << "\nBest hit rate: " << bestHitRate->configName 
                  << " (" << (bestHitRate->overallHitRate * 100) << "%)" << std::endl;
        
        std::cout << "Fastest simulation: " << bestSpeed->configName 
                  << " (" << (bestSpeed->accessesPerSecond / 1e6) << "M accesses/s)" << std::endl;
        
        std::cout << "Best average access time: " << bestAvgTime->configName 
                  << " (" << bestAvgTime->avgAccessTime << " cycles)" << std::endl;
        
        // Specific recommendations
        std::cout << "\nSpecific Recommendations:" << std::endl;
        
        // Check if victim cache helped
        for (const auto& result : results) {
            if (result.configName.find("Victim Cache") != std::string::npos) {
                if (result.victimCacheHitRate > 0.1) {
                    std::cout << "- Victim cache is effective (hit rate: " 
                              << (result.victimCacheHitRate * 100) 
                              << "%), recommended for small L1 caches" << std::endl;
                }
            }
        }
        
        // Check NRU performance
        for (const auto& result : results) {
            if (result.configName.find("NRU") != std::string::npos) {
                auto lruResult = std::find_if(results.begin(), results.end(),
                    [](const auto& r) { 
                        return r.configName.find("Basic") != std::string::npos;
                    });
                
                if (lruResult != results.end()) {
                    double diff = result.overallHitRate - lruResult->overallHitRate;
                    if (std::abs(diff) < 0.01) {
                        std::cout << "- NRU performs similarly to LRU but with lower overhead" << std::endl;
                    }
                }
            }
        }
    }
    
private:
    std::vector<std::pair<std::string, MemoryHierarchyConfig>> configurations_;
    
    SimulationResult runSingleSimulation(const std::string& name,
                                       const MemoryHierarchyConfig& config,
                                       const std::string& traceFile) {
        SimulationResult result;
        result.configName = name;
        result.config = config;
        
        // Start timing
        auto startTime = high_resolution_clock::now();
        
        // Create memory hierarchy
        MemoryHierarchy hierarchy(config);
        
        // Create victim cache if needed
        std::unique_ptr<VictimCache> victimCache;
        if (name.find("Victim Cache") != std::string::npos) {
            victimCache = std::make_unique<VictimCache>(8);
        }
        
        // Parse and process trace
        TraceParser parser(traceFile);
        result.totalAccesses = 0;
        
        while (auto access = parser.getNextAccess()) {
            hierarchy.access(access->address, access->isWrite);
            result.totalAccesses++;
            
            // Simulate victim cache if present
            if (victimCache && result.totalAccesses % 10 == 0) {
                // Simplified victim cache simulation
                if (victimCache->findBlock(access->address)) {
                    result.victimCacheHits++;
                }
            }
        }
        
        // End timing
        auto endTime = high_resolution_clock::now();
        result.simulationTimeMs = duration_cast<milliseconds>(endTime - startTime).count();
        
        // Collect statistics
        result.l1Hits = hierarchy.getL1Cache() ? 
            hierarchy.getTotalMemoryAccesses() - hierarchy.getL1Misses() : 0;
        result.l1Misses = hierarchy.getL1Misses();
        result.l1HitRate = 1.0 - hierarchy.getL1MissRate();
        
        if (config.l2Config) {
            result.l2HitRate = hierarchy.getL2HitRate();
            // Approximate L2 hits
            result.l2Hits = result.l1Misses * result.l2HitRate;
            result.l2Misses = result.l1Misses * (1 - result.l2HitRate);
        } else {
            result.l2HitRate = 0.0;
            result.l2Hits = 0;
            result.l2Misses = result.l1Misses;
        }
        
        // Calculate overall hit rate
        double memoryAccesses = result.l2Misses;
        result.overallHitRate = 1.0 - (memoryAccesses / result.totalAccesses);
        
        // Calculate average access time (simplified model)
        double l1Time = 1.0;
        double l2Time = 10.0;
        double memTime = 100.0;
        
        result.avgAccessTime = (result.l1Hits * l1Time + 
                               result.l2Hits * l2Time + 
                               memoryAccesses * memTime) / result.totalAccesses;
        
        // Other stats
        result.writebacks = hierarchy.getL1Cache() ? 
            (*hierarchy.getL1Cache())->getWriteBacks() : 0;
        
        if (victimCache) {
            result.victimCacheHitRate = victimCache->getHitRate();
        }
        
        // Performance metrics
        result.accessesPerSecond = result.simulationTimeMs > 0 ?
            (result.totalAccesses * 1000.0 / result.simulationTimeMs) : 0;
        
        return result;
    }
    
    void printProgress(size_t completed, size_t total) {
        int barWidth = 50;
        float progress = (float)completed / total;
        
        std::cout << "\r[";
        int pos = barWidth * progress;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << "%" << std::flush;
    }
};

void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " [options] <trace_file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help            Show this help message" << std::endl;
    std::cout << "  -c, --config <file>   Add custom configuration from JSON file" << std::endl;
    std::cout << "  -d, --default         Use default configurations only" << std::endl;
    std::cout << "  -p, --parallel        Run simulations in parallel (default)" << std::endl;
    std::cout << "  -s, --sequential      Run simulations sequentially" << std::endl;
    std::cout << "  -o, --output <file>   Export results to CSV file" << std::endl;
    std::cout << "  -g, --graphs          Generate comparison charts" << std::endl;
    std::cout << "  -r, --recommendations Show recommendations based on results" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " trace.txt" << std::endl;
    std::cout << "  " << programName << " -g -r -o results.csv trace.txt" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse command line
    std::string traceFile;
    std::string outputFile;
    std::vector<std::string> configFiles;
    bool useDefaults = true;
    bool parallel = true;
    bool showGraphs = false;
    bool showRecommendations = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-d" || arg == "--default") {
            useDefaults = true;
        } else if (arg == "-p" || arg == "--parallel") {
            parallel = true;
        } else if (arg == "-s" || arg == "--sequential") {
            parallel = false;
        } else if (arg == "-g" || arg == "--graphs") {
            showGraphs = true;
        } else if (arg == "-r" || arg == "--recommendations") {
            showRecommendations = true;
        } else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            configFiles.push_back(argv[++i]);
            useDefaults = false;
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg[0] != '-') {
            traceFile = arg;
        }
    }
    
    if (traceFile.empty()) {
        std::cerr << "Error: No trace file specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    if (!std::filesystem::exists(traceFile)) {
        std::cerr << "Error: Trace file does not exist: " << traceFile << std::endl;
        return 1;
    }
    
    try {
        PerformanceComparison comparison;
        
        // Add configurations
        if (useDefaults) {
            comparison.addDefaultConfigurations();
        }
        
        // Add custom configurations
        for (const auto& configFile : configFiles) {
            ConfigManager configMgr(ConfigManager::ConfigFormat::JSON);
            auto config = configMgr.loadConfig(configFile);
            if (config) {
                comparison.addConfiguration(
                    std::filesystem::path(configFile).stem().string(),
                    config->hierarchyConfig
                );
            }
        }
        
        // Run comparisons
        auto results = comparison.runComparisons(traceFile, parallel);
        
        // Display results
        comparison.printComparisonTable(results);
        
        if (showGraphs) {
            comparison.generateCharts(results);
        }
        
        if (showRecommendations) {
            comparison.generateRecommendations(results);
        }
        
        if (!outputFile.empty()) {
            comparison.exportResults(results, outputFile);
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
