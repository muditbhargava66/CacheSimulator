#include "adaptive_prefetcher.h"
#include <algorithm>
#include <iostream>

namespace cachesim {

AdaptivePrefetcher::AdaptivePrefetcher(int maxDistance, Strategy initialStrategy)
    : prefetchDistance(1), 
      maxPrefetchDistance(maxDistance),
      usefulPrefetches(0), 
      uselessPrefetches(0),
      totalCacheMisses(0),
      currentStrategy(initialStrategy),
      sequentialConfidence(0.5),
      strideConfidence(0.5) {
    
    addressHistory.resize(historySize, 0);
}

void AdaptivePrefetcher::recordPrefetchOutcome(bool useful) {
    if (useful) {
        usefulPrefetches++;
        updateConfidence(currentStrategy, true);
    } else {
        uselessPrefetches++;
        updateConfidence(currentStrategy, false);
    }
}

void AdaptivePrefetcher::updateConfidence(Strategy strategy, bool wasUseful) {
    // Learning rate for confidence updates
    constexpr double alpha = 0.1;
    
    if (strategy == Strategy::Sequential) {
        sequentialConfidence = sequentialConfidence * (1 - alpha) + 
                              (wasUseful ? alpha : 0);
    } else if (strategy == Strategy::Stride) {
        strideConfidence = strideConfidence * (1 - alpha) + 
                          (wasUseful ? alpha : 0);
    }
}

AdaptivePrefetcher::Strategy AdaptivePrefetcher::chooseBestStrategy() const {
    if (sequentialConfidence > strideConfidence) {
        return Strategy::Sequential;
    } else {
        return Strategy::Stride;
    }
}

void AdaptivePrefetcher::adaptPrefetchDistance() {
    if (usefulPrefetches + uselessPrefetches == 0) {
        return; // No data yet
    }
    
    // Calculate prefetch accuracy
    double prefetchAccuracy = getPrefetchAccuracy();
    
    // Adjust prefetch distance based on accuracy
    if (prefetchAccuracy > 0.8) {
        // High accuracy - be more aggressive
        prefetchDistance = std::min(prefetchDistance * 2, maxPrefetchDistance);
    } else if (prefetchAccuracy < 0.5) {
        // Low accuracy - be more conservative
        prefetchDistance = std::max(prefetchDistance / 2, 1);
    }
    
    // If using adaptive strategy, choose the best strategy based on confidence
    if (currentStrategy == Strategy::Adaptive) {
        currentStrategy = chooseBestStrategy();
    }
}

void AdaptivePrefetcher::updateHistory(uint32_t address) {
    // Record address for history-based pattern detection
    addressHistory.push_front(address);
    addressHistory.pop_back();
    
    // Update stride detection
    uint32_t pc = address & 0xFFFF0000; // Use high bits as "PC" for simplicity
    
    if (lastAccessMap.find(pc) != lastAccessMap.end()) {
        int32_t stride = address - lastAccessMap[pc];
        strideMap[pc] = stride;
    }
    
    lastAccessMap[pc] = address;
    
    // Count total cache misses for coverage calculation
    totalCacheMisses++;
}

int32_t AdaptivePrefetcher::detectStride(uint32_t address) {
    uint32_t pc = address & 0xFFFF0000; // Use high bits as "PC" for simplicity
    
    if (strideMap.find(pc) != strideMap.end()) {
        return strideMap[pc];
    }
    
    return 0; // No stride detected
}

uint32_t AdaptivePrefetcher::getPrefetchAddress(uint32_t currentAddress, int32_t detectedStride) {
    // Determine the strategy to use for prefetching
    Strategy strategyToUse = currentStrategy;
    
    // Adaptive strategy dynamically selects between sequential and stride
    if (strategyToUse == Strategy::Adaptive) {
        strategyToUse = chooseBestStrategy();
    }
    
    // Calculate prefetch address based on the selected strategy
    switch (strategyToUse) {
            case Strategy::Sequential:
                // Sequential prefetching: prefetch next N blocks
                return currentAddress + prefetchDistance;
                
            case Strategy::Stride: {
                // Stride-based prefetching
                int32_t stride = (detectedStride != 0) ? detectedStride : detectStride(currentAddress);
                
                if (stride != 0) {
                    // Use detected stride
                    return currentAddress + (stride * prefetchDistance);
                } else {
                    // Fall back to sequential if no stride detected
                    return currentAddress + prefetchDistance;
                }
            }
                
            case Strategy::Adaptive:
            default:
                // Fallback to sequential prefetching
                return currentAddress + prefetchDistance;
    }
}

double AdaptivePrefetcher::getPrefetchAccuracy() const {
    if (usefulPrefetches + uselessPrefetches == 0) {
        return 0.0;
    }
    
    return static_cast<double>(usefulPrefetches) / (usefulPrefetches + uselessPrefetches);
}

double AdaptivePrefetcher::getPrefetchCoverage() const {
    if (totalCacheMisses == 0) {
        return 0.0;
    }
    
    return static_cast<double>(usefulPrefetches) / totalCacheMisses;
}

void AdaptivePrefetcher::printStats() const {
    std::cout << "Prefetcher Statistics:" << std::endl;
    std::cout << "  Strategy: ";
    
    switch (currentStrategy) {
        case Strategy::Sequential:
            std::cout << "Sequential";
            break;
        case Strategy::Stride:
            std::cout << "Stride-based";
            break;
        case Strategy::Adaptive:
            std::cout << "Adaptive";
            break;
    }
    
    std::cout << std::endl;
    std::cout << "  Prefetch Distance: " << prefetchDistance << std::endl;
    std::cout << "  Useful Prefetches: " << usefulPrefetches << std::endl;
    std::cout << "  Useless Prefetches: " << uselessPrefetches << std::endl;
    std::cout << "  Prefetch Accuracy: " << (getPrefetchAccuracy() * 100.0) << "%" << std::endl;
    std::cout << "  Prefetch Coverage: " << (getPrefetchCoverage() * 100.0) << "%" << std::endl;
    std::cout << "  Sequential Confidence: " << (sequentialConfidence * 100.0) << "%" << std::endl;
    std::cout << "  Stride Confidence: " << (strideConfidence * 100.0) << "%" << std::endl;
}

void AdaptivePrefetcher::resetStats() {
    usefulPrefetches = 0;
    uselessPrefetches = 0;
    totalCacheMisses = 0;
}

} // namespace cachesim