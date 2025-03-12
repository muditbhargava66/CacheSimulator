#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <variant>

#include "trace_parser.h"

namespace cachesim {

// Structure for trace statistics
struct TraceStatistics {
    size_t totalAccesses = 0;
    size_t readAccesses = 0;
    size_t writeAccesses = 0;
    
    // Advanced statistics
    struct AddressRange {
        uint32_t start;
        uint32_t end;
        size_t accesses;
    };
    
    std::vector<AddressRange> hotRanges;  // Most frequently accessed ranges
    std::unordered_map<uint32_t, size_t> accessFrequency; // Access count per address
    
    // Pattern detection results
    enum class PatternType {
        Sequential,
        Strided,
        Random,
        LoopingAccess,
        Mixed
    };
    
    struct PatternInfo {
        PatternType type;
        double confidence;  // 0.0 to 1.0
        int32_t stride;     // For strided patterns
        size_t loopSize;    // For looping patterns
    };
    
    std::optional<PatternInfo> detectedPattern;
};

// Utility class for analyzing trace files
class TraceAnalyzer {
public:
    // Constructor with path
    explicit TraceAnalyzer(const std::filesystem::path& filepath);
    
    // Analyze the entire trace file
    [[nodiscard]] TraceStatistics analyzeTrace();
    
    // Detect memory access patterns
    [[nodiscard]] std::optional<TraceStatistics::PatternInfo> detectPattern();
    
    // Get most frequently accessed memory ranges
    [[nodiscard]] std::vector<TraceStatistics::AddressRange> getHotRanges(size_t numRanges = 5, uint32_t rangeSize = 4096);
    
    // Print analysis results
    void printAnalysis(const TraceStatistics& stats) const;
    
    // Generate a new trace file with filtered accesses
    bool generateFilteredTrace(const std::filesystem::path& outputPath, 
                              const std::function<bool(const MemoryAccess&)>& filter);
    
    // Check if file is valid
    [[nodiscard]] bool isValid() const { return parser.isValid(); }

private:
    TraceParser parser;
    
    // Helper methods for pattern detection
    [[nodiscard]] bool isSequentialPattern(const std::vector<MemoryAccess>& accesses, double& confidence) const;
    [[nodiscard]] std::optional<int32_t> detectStride(const std::vector<MemoryAccess>& accesses, double& confidence) const;
    [[nodiscard]] bool isLoopingPattern(const std::vector<MemoryAccess>& accesses, size_t& loopSize, double& confidence) const;
};

// Utility functions for trace file generation
namespace trace_generator {
    // Generate a sequential access pattern trace
    bool generateSequentialTrace(const std::filesystem::path& outputPath, 
                                uint32_t startAddress, 
                                size_t count, 
                                uint32_t stride = 4,
                                double writeRatio = 0.2);
    
    // Generate a random access pattern trace
    bool generateRandomTrace(const std::filesystem::path& outputPath,
                            uint32_t minAddress,
                            uint32_t maxAddress,
                            size_t count,
                            double writeRatio = 0.2);
    
    // Generate a trace with spatial locality
    bool generateLocalityTrace(const std::filesystem::path& outputPath,
                              size_t numRegions,
                              uint32_t regionSize,
                              size_t accessesPerRegion,
                              double writeRatio = 0.2);
    
    // Generate a mixed pattern trace
    bool generateMixedTrace(const std::filesystem::path& outputPath,
                           const std::vector<std::pair<std::string, double>>& patternMix,
                           size_t totalAccesses,
                           double writeRatio = 0.2);
}

} // namespace cachesim