/**
 * @file cache_analyzer.cpp
 * @brief Advanced cache analysis tool for v1.2.0
 * @author Mudit Bhargava
 * @date 2025-06-01
 * @version 1.2.0
 * 
 * This tool provides comprehensive analysis of cache behavior including:
 * - Working set analysis
 * - Reuse distance calculation
 * - Access pattern classification
 * - Optimal cache size recommendation
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include "../src/utils/trace_parser.h"
#include "../src/utils/visualization.h"
#include "../src/utils/statistics.h"

namespace fs = std::filesystem;
using namespace cachesim;
using namespace std::chrono;

class CacheAnalyzer {
public:
    struct AnalysisResult {
        // Working set analysis
        std::vector<std::pair<size_t, size_t>> workingSetSizes;  // (window_size, unique_addrs)
        
        // Reuse distance
        std::vector<size_t> reuseDistances;
        double avgReuseDistance;
        double medianReuseDistance;
        
        // Access patterns
        double sequentialRatio;
        double strideRatio;
        double randomRatio;
        std::unordered_map<int32_t, size_t> strideFrequency;
        
        // Temporal locality
        double temporalLocality;  // Probability of reuse
        std::vector<std::pair<uint32_t, size_t>> hotAddresses;  // Most accessed
        
        // Spatial locality
        double spatialLocality;   // Adjacent block access probability
        size_t avgSpatialDistance;
        
        // Cache recommendations
        struct CacheRecommendation {
            size_t size;
            size_t associativity;
            size_t blockSize;
            double expectedHitRate;
            std::string reasoning;
        };
        std::vector<CacheRecommendation> recommendations;
        
        // Statistics
        size_t totalAccesses;
        size_t uniqueAddresses;
        size_t readCount;
        size_t writeCount;
        double readWriteRatio;
    };
    
    AnalysisResult analyzeTrace(const std::string& traceFile) {
        std::cout << "Analyzing trace file: " << traceFile << std::endl;
        auto startTime = high_resolution_clock::now();
        
        AnalysisResult result{};
        
        // Parse trace
        TraceParser parser(traceFile);
        std::vector<MemoryAccess> accesses = parser.parseAll();
        
        result.totalAccesses = accesses.size();
        result.readCount = parser.getReadAccesses();
        result.writeCount = parser.getWriteAccesses();
        result.readWriteRatio = result.writeCount > 0 ? 
            (double)result.readCount / result.writeCount : 0.0;
        
        // Extract addresses
        std::vector<uint32_t> addresses;
        addresses.reserve(accesses.size());
        for (const auto& access : accesses) {
            addresses.push_back(access.address);
        }
        
        // Count unique addresses
        std::unordered_set<uint32_t> uniqueAddrs(addresses.begin(), addresses.end());
        result.uniqueAddresses = uniqueAddrs.size();
        
        // Analyze components
        analyzeWorkingSet(addresses, result);
        analyzeReuseDistance(addresses, result);
        analyzeAccessPatterns(addresses, result);
        analyzeLocality(addresses, result);
        generateRecommendations(result);
        
        auto endTime = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(endTime - startTime).count();
        
        std::cout << "Analysis completed in " << duration << " ms" << std::endl;
        
        return result;
    }
    
    void printReport(const AnalysisResult& result, bool verbose = true) {
        std::cout << "\n=== Cache Analysis Report ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        
        // Basic statistics
        std::cout << "\nBasic Statistics:" << std::endl;
        std::cout << "  Total accesses: " << result.totalAccesses << std::endl;
        std::cout << "  Unique addresses: " << result.uniqueAddresses << std::endl;
        std::cout << "  Reads: " << result.readCount 
                  << " (" << (result.readCount * 100.0 / result.totalAccesses) << "%)" << std::endl;
        std::cout << "  Writes: " << result.writeCount 
                  << " (" << (result.writeCount * 100.0 / result.totalAccesses) << "%)" << std::endl;
        
        // Working set
        std::cout << "\nWorking Set Analysis:" << std::endl;
        for (const auto& [window, size] : result.workingSetSizes) {
            std::cout << "  Window " << std::setw(6) << window << ": " 
                      << std::setw(6) << size << " unique addresses" << std::endl;
        }
        
        // Reuse distance
        std::cout << "\nReuse Distance:" << std::endl;
        std::cout << "  Average: " << result.avgReuseDistance << std::endl;
        std::cout << "  Median: " << result.medianReuseDistance << std::endl;
        
        if (verbose) {
            // Reuse distance histogram
            std::cout << "\n  Distribution:" << std::endl;
            printReuseHistogram(result.reuseDistances);
        }
        
        // Access patterns
        std::cout << "\nAccess Patterns:" << std::endl;
        std::cout << "  Sequential: " << (result.sequentialRatio * 100) << "%" << std::endl;
        std::cout << "  Strided: " << (result.strideRatio * 100) << "%" << std::endl;
        std::cout << "  Random: " << (result.randomRatio * 100) << "%" << std::endl;
        
        if (verbose && !result.strideFrequency.empty()) {
            std::cout << "\n  Top strides:" << std::endl;
            printTopStrides(result.strideFrequency, 5);
        }
        
        // Locality
        std::cout << "\nLocality Analysis:" << std::endl;
        std::cout << "  Temporal locality: " << (result.temporalLocality * 100) << "%" << std::endl;
        std::cout << "  Spatial locality: " << (result.spatialLocality * 100) << "%" << std::endl;
        std::cout << "  Avg spatial distance: " << result.avgSpatialDistance << " bytes" << std::endl;
        
        if (verbose && !result.hotAddresses.empty()) {
            std::cout << "\n  Hottest addresses:" << std::endl;
            for (size_t i = 0; i < std::min(size_t(5), result.hotAddresses.size()); ++i) {
                std::cout << "    0x" << std::hex << result.hotAddresses[i].first 
                          << std::dec << ": " << result.hotAddresses[i].second 
                          << " accesses" << std::endl;
            }
        }
        
        // Recommendations
        std::cout << "\nCache Configuration Recommendations:" << std::endl;
        for (size_t i = 0; i < result.recommendations.size(); ++i) {
            const auto& rec = result.recommendations[i];
            std::cout << "\n  Option " << (i + 1) << ":" << std::endl;
            std::cout << "    Size: " << (rec.size / 1024) << " KB" << std::endl;
            std::cout << "    Associativity: " << rec.associativity << "-way" << std::endl;
            std::cout << "    Block size: " << rec.blockSize << " bytes" << std::endl;
            std::cout << "    Expected hit rate: " << (rec.expectedHitRate * 100) << "%" << std::endl;
            std::cout << "    Reasoning: " << rec.reasoning << std::endl;
        }
    }
    
    void exportResults(const AnalysisResult& result, const std::string& outputFile) {
        std::ofstream out(outputFile);
        
        out << "metric,value\n";
        out << "total_accesses," << result.totalAccesses << "\n";
        out << "unique_addresses," << result.uniqueAddresses << "\n";
        out << "read_count," << result.readCount << "\n";
        out << "write_count," << result.writeCount << "\n";
        out << "avg_reuse_distance," << result.avgReuseDistance << "\n";
        out << "temporal_locality," << result.temporalLocality << "\n";
        out << "spatial_locality," << result.spatialLocality << "\n";
        out << "sequential_ratio," << result.sequentialRatio << "\n";
        out << "stride_ratio," << result.strideRatio << "\n";
        out << "random_ratio," << result.randomRatio << "\n";
        
        std::cout << "Results exported to " << outputFile << std::endl;
    }
    
    void generateVisualizations(const AnalysisResult& result) {
        // Working set curve
        std::cout << "\nWorking Set Size Curve:" << std::endl;
        std::vector<std::pair<double, double>> wsData;
        for (const auto& [window, size] : result.workingSetSizes) {
            wsData.push_back({std::log10(window), size / 1024.0});  // KB
        }
        std::cout << Visualization::generateLineChart(
            wsData, 60, 15,
            "Working Set Size vs Window",
            "Window Size (log10)", "Working Set (KB)"
        );
        
        // Access pattern breakdown
        std::cout << "\nAccess Pattern Distribution:" << std::endl;
        std::vector<std::pair<std::string, double>> patterns = {
            {"Sequential", result.sequentialRatio * result.totalAccesses},
            {"Strided", result.strideRatio * result.totalAccesses},
            {"Random", result.randomRatio * result.totalAccesses}
        };
        std::cout << Visualization::generatePieChart(patterns, 10);
        
        // Locality metrics
        std::cout << "\nLocality Metrics:" << std::endl;
        std::vector<std::pair<std::string, double>> locality = {
            {"Temporal Locality", result.temporalLocality * 100},
            {"Spatial Locality", result.spatialLocality * 100},
            {"No Locality", (1 - std::max(result.temporalLocality, result.spatialLocality)) * 100}
        };
        std::cout << Visualization::generateHistogram(locality, 40);
    }
    
private:
    void analyzeWorkingSet(const std::vector<uint32_t>& addresses, AnalysisResult& result) {
        // Analyze working set for different window sizes
        std::vector<size_t> windowSizes = {100, 500, 1000, 5000, 10000, 50000};
        
        for (size_t window : windowSizes) {
            if (window > addresses.size()) break;
            
            std::unordered_set<uint32_t> uniqueInWindow;
            
            // Initial window
            for (size_t i = 0; i < window && i < addresses.size(); ++i) {
                uniqueInWindow.insert(addresses[i]);
            }
            
            // Slide window and track max unique addresses
            size_t maxUnique = uniqueInWindow.size();
            
            for (size_t i = window; i < addresses.size(); ++i) {
                uniqueInWindow.erase(addresses[i - window]);
                uniqueInWindow.insert(addresses[i]);
                maxUnique = std::max(maxUnique, uniqueInWindow.size());
            }
            
            result.workingSetSizes.push_back({window, maxUnique});
        }
    }
    
    void analyzeReuseDistance(const std::vector<uint32_t>& addresses, AnalysisResult& result) {
        std::unordered_map<uint32_t, size_t> lastSeen;
        
        for (size_t i = 0; i < addresses.size(); ++i) {
            uint32_t addr = addresses[i];
            
            if (lastSeen.find(addr) != lastSeen.end()) {
                size_t distance = i - lastSeen[addr];
                result.reuseDistances.push_back(distance);
            }
            
            lastSeen[addr] = i;
        }
        
        if (!result.reuseDistances.empty()) {
            // Calculate average
            result.avgReuseDistance = std::accumulate(
                result.reuseDistances.begin(), result.reuseDistances.end(), 0.0
            ) / result.reuseDistances.size();
            
            // Calculate median
            std::vector<size_t> sorted = result.reuseDistances;
            std::sort(sorted.begin(), sorted.end());
            result.medianReuseDistance = sorted[sorted.size() / 2];
        }
    }
    
    void analyzeAccessPatterns(const std::vector<uint32_t>& addresses, AnalysisResult& result) {
        if (addresses.size() < 2) return;
        
        size_t sequential = 0;
        size_t strided = 0;
        std::unordered_map<int32_t, size_t> strides;
        
        // Analyze consecutive accesses
        for (size_t i = 1; i < addresses.size(); ++i) {
            int32_t stride = (int32_t)addresses[i] - (int32_t)addresses[i-1];
            
            if (std::abs(stride) <= 64) {  // Cache line size
                sequential++;
            } else if (std::abs(stride) <= 4096 && stride % 64 == 0) {  // Regular stride
                strided++;
                strides[stride]++;
            }
        }
        
        size_t total = addresses.size() - 1;
        result.sequentialRatio = (double)sequential / total;
        result.strideRatio = (double)strided / total;
        result.randomRatio = 1.0 - result.sequentialRatio - result.strideRatio;
        result.strideFrequency = strides;
    }
    
    void analyzeLocality(const std::vector<uint32_t>& addresses, AnalysisResult& result) {
        std::unordered_map<uint32_t, size_t> accessCount;
        size_t reuses = 0;
        size_t spatialHits = 0;
        std::vector<size_t> spatialDistances;
        
        for (size_t i = 0; i < addresses.size(); ++i) {
            uint32_t addr = addresses[i];
            uint32_t blockAddr = addr & ~0x3F;  // 64-byte blocks
            
            // Temporal locality
            if (accessCount[addr] > 0) {
                reuses++;
            }
            accessCount[addr]++;
            
            // Spatial locality
            if (i > 0) {
                uint32_t prevBlock = addresses[i-1] & ~0x3F;
                if (blockAddr == prevBlock || 
                    blockAddr == prevBlock + 64 || 
                    blockAddr == prevBlock - 64) {
                    spatialHits++;
                }
                
                size_t distance = std::abs((int32_t)addr - (int32_t)addresses[i-1]);
                spatialDistances.push_back(distance);
            }
        }
        
        result.temporalLocality = (double)reuses / addresses.size();
        result.spatialLocality = addresses.size() > 1 ? 
            (double)spatialHits / (addresses.size() - 1) : 0.0;
        
        if (!spatialDistances.empty()) {
            result.avgSpatialDistance = std::accumulate(
                spatialDistances.begin(), spatialDistances.end(), 0ULL
            ) / spatialDistances.size();
        }
        
        // Find hot addresses
        std::vector<std::pair<uint32_t, size_t>> addrCounts(
            accessCount.begin(), accessCount.end()
        );
        std::sort(addrCounts.begin(), addrCounts.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; }
        );
        
        result.hotAddresses.assign(
            addrCounts.begin(), 
            addrCounts.begin() + std::min(size_t(10), addrCounts.size())
        );
    }
    
    void generateRecommendations(AnalysisResult& result) {
        // Based on working set analysis
        size_t recommendedSize = 0;
        for (const auto& [window, size] : result.workingSetSizes) {
            if (window >= 1000 && window <= 10000) {
                recommendedSize = std::max(recommendedSize, size * 64);  // Convert to bytes
            }
        }
        
        // Round up to power of 2
        recommendedSize = 1ULL << (size_t)std::ceil(std::log2(recommendedSize));
        
        // Recommendation 1: Optimized for working set
        {
            AnalysisResult::CacheRecommendation rec;
            rec.size = recommendedSize;
            rec.associativity = result.randomRatio > 0.5 ? 8 : 4;
            rec.blockSize = result.spatialLocality > 0.3 ? 128 : 64;
            rec.expectedHitRate = std::min(0.95, result.temporalLocality + result.spatialLocality * 0.5);
            rec.reasoning = "Sized to fit primary working set with associativity based on access pattern";
            result.recommendations.push_back(rec);
        }
        
        // Recommendation 2: High performance
        {
            AnalysisResult::CacheRecommendation rec;
            rec.size = recommendedSize * 2;
            rec.associativity = 16;
            rec.blockSize = 64;
            rec.expectedHitRate = std::min(0.98, rec.expectedHitRate + 0.05);
            rec.reasoning = "Larger cache with high associativity for maximum hit rate";
            result.recommendations.push_back(rec);
        }
        
        // Recommendation 3: Cost-effective
        {
            AnalysisResult::CacheRecommendation rec;
            rec.size = recommendedSize / 2;
            rec.associativity = 2;
            rec.blockSize = result.spatialLocality > 0.5 ? 128 : 64;
            rec.expectedHitRate = result.temporalLocality * 0.8;
            rec.reasoning = "Smaller cache optimized for cost with victim cache recommended";
            result.recommendations.push_back(rec);
        }
    }
    
    void printReuseHistogram(const std::vector<size_t>& distances) {
        if (distances.empty()) return;
        
        // Create buckets
        std::vector<std::pair<std::string, double>> buckets = {
            {"1-10", 0},
            {"11-100", 0},
            {"101-1K", 0},
            {"1K-10K", 0},
            {"10K-100K", 0},
            {">100K", 0}
        };
        
        for (size_t dist : distances) {
            if (dist <= 10) buckets[0].second++;
            else if (dist <= 100) buckets[1].second++;
            else if (dist <= 1000) buckets[2].second++;
            else if (dist <= 10000) buckets[3].second++;
            else if (dist <= 100000) buckets[4].second++;
            else buckets[5].second++;
        }
        
        std::cout << Visualization::generateHistogram(buckets, 40);
    }
    
    void printTopStrides(const std::unordered_map<int32_t, size_t>& strides, size_t n) {
        std::vector<std::pair<int32_t, size_t>> sorted(strides.begin(), strides.end());
        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; }
        );
        
        for (size_t i = 0; i < std::min(n, sorted.size()); ++i) {
            std::cout << "    " << std::setw(6) << sorted[i].first 
                      << " bytes: " << sorted[i].second << " occurrences" << std::endl;
        }
    }
};

void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " [options] <trace_file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -v, --verbose           Enable verbose output" << std::endl;
    std::cout << "  -o, --output <file>     Export results to CSV file" << std::endl;
    std::cout << "  -g, --graphs            Generate visualizations" << std::endl;
    std::cout << "  --no-recommendations    Skip cache recommendations" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " trace.txt" << std::endl;
    std::cout << "  " << programName << " -v -g -o results.csv trace.txt" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse command line
    std::string traceFile;
    std::string outputFile;
    bool verbose = false;
    bool showGraphs = false;
    bool showRecommendations = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-g" || arg == "--graphs") {
            showGraphs = true;
        } else if (arg == "--no-recommendations") {
            showRecommendations = false;
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
    
    if (!fs::exists(traceFile)) {
        std::cerr << "Error: Trace file does not exist: " << traceFile << std::endl;
        return 1;
    }
    
    try {
        CacheAnalyzer analyzer;
        auto result = analyzer.analyzeTrace(traceFile);
        
        analyzer.printReport(result, verbose);
        
        if (showGraphs) {
            analyzer.generateVisualizations(result);
        }
        
        if (!outputFile.empty()) {
            analyzer.exportResults(result, outputFile);
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
