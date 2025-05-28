#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cache.h"
#include "adaptive_prefetcher.h"
#include "stride_predictor.h"

namespace cachesim {

// Memory hierarchy configuration struct
struct MemoryHierarchyConfig {
    CacheConfig l1Config;
    std::optional<CacheConfig> l2Config;
    bool useStridePrediction = false;
    bool useAdaptivePrefetching = false;
    int strideTableSize = 1024;
    
    // Default constructor
    MemoryHierarchyConfig() = default;
    
    // Constructor with parameters
    MemoryHierarchyConfig(
        const CacheConfig& l1Config,
        const std::optional<CacheConfig>& l2Config = std::nullopt,
        bool useStridePrediction = false,
        bool useAdaptivePrefetching = false,
        int strideTableSize = 1024
    ) : l1Config(l1Config), 
        l2Config(l2Config), 
        useStridePrediction(useStridePrediction),
        useAdaptivePrefetching(useAdaptivePrefetching),
        strideTableSize(strideTableSize) {}
};

// Represents the entire memory hierarchy with multiple cache levels
class MemoryHierarchy {
public:
    // Constructor with configuration
    explicit MemoryHierarchy(const MemoryHierarchyConfig& config);
    
    // Process a memory access
    void access(uint32_t address, bool isWrite);
    
    // Process multiple memory accesses from a trace
    void processTrace(const std::string& traceFile);
    
    // Print memory hierarchy statistics
    void printStats() const;
    
    // Reset all statistics
    void resetStats();
    
    // Compare performance with another hierarchy
    void compareWith(const MemoryHierarchy& other) const;
    
    // Getters for statistics
    [[nodiscard]] int getL1Misses() const { return l1Misses; }
    [[nodiscard]] int getL1Reads() const { return l1Reads; }
    [[nodiscard]] int getL1Writes() const { return l1Writes; }
    [[nodiscard]] int getPrefetchDistance() const { return prefetchDistance; }
    [[nodiscard]] int getTotalMemoryAccesses() const { return l1Reads + l1Writes; }
    [[nodiscard]] double getL1MissRate() const;
    [[nodiscard]] double getPrefetchAccuracy() const;
    [[nodiscard]] std::string getPrefetchStats() const;

private:
    // Cache instances
    Cache l1;
    std::optional<Cache> l2;
    
    // Prefetching components
    StridePredictor stridePredictor;
    std::optional<AdaptivePrefetcher> adaptivePrefetcher;
    int prefetchDistance;
    bool prefetchEnabled;
    
    // Statistics
    int l1Reads = 0;
    int l1Writes = 0;
    int l1Misses = 0;
    int usefulPrefetches = 0;
    int uselessPrefetches = 0;
    
    // Helper methods
    void recordPrefetchOutcome(bool useful);
    void adaptPrefetchDistance();
};

} // namespace cachesim