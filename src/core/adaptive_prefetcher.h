#pragma once

#include <cstdint>
#include <cmath>
#include <deque>
#include <unordered_map>

namespace cachesim {

// Class that implements adaptive prefetching with various strategies
class AdaptivePrefetcher {
public:
    enum class Strategy {
        Sequential,  // Simple sequential prefetching
        Stride,      // Stride-based prefetching
        Adaptive     // Adaptively choose between strategies based on performance
    };

    AdaptivePrefetcher(int maxDistance = 16, Strategy initialStrategy = Strategy::Adaptive);
    
    // Record prefetch hit or miss for adaptation
    void recordPrefetchOutcome(bool useful);
    
    // Adapt prefetching strategy and distance based on recorded outcomes
    void adaptPrefetchDistance();
    
    // Get prefetch address based on current strategy and history
    uint32_t getPrefetchAddress(uint32_t currentAddress, int32_t detectedStride = 0);
    
    // Update history with a new address
    void updateHistory(uint32_t address);
    
    // Get current prefetch distance
    [[nodiscard]] int getPrefetchDistance() const { return prefetchDistance; }
    
    // Get current strategy
    [[nodiscard]] Strategy getStrategy() const { return currentStrategy; }
    
    // Get prefetch accuracy
    [[nodiscard]] double getPrefetchAccuracy() const;
    
    // Get prefetch coverage
    [[nodiscard]] double getPrefetchCoverage() const;
    
    // Print statistics
    void printStats() const;
    
    // Reset statistics
    void resetStats();

private:
    int prefetchDistance;
    int maxPrefetchDistance;
    int usefulPrefetches;
    int uselessPrefetches;
    int totalCacheMisses;
    Strategy currentStrategy;
    
    // Access history for pattern detection
    std::deque<uint32_t> addressHistory;
    static constexpr int historySize = 32;
    
    // Stride detection table
    std::unordered_map<uint32_t, uint32_t> lastAccessMap;
    std::unordered_map<uint32_t, int32_t> strideMap;
    
    // Confidence tracking for different strategies
    double sequentialConfidence;
    double strideConfidence;
    
    // Helper methods
    int32_t detectStride(uint32_t address);
    void updateConfidence(Strategy strategy, bool wasUseful);
    Strategy chooseBestStrategy() const;
};

} // namespace cachesim