#include "memory_hierarchy.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <stdexcept>

#include "../utils/trace_parser.h"

namespace cachesim {

MemoryHierarchy::MemoryHierarchy(const MemoryHierarchyConfig& config)
    : l1(config.l1Config),
      stridePredictor(config.strideTableSize),
      prefetchDistance(config.l1Config.prefetchDistance),
      prefetchEnabled(config.l1Config.prefetchEnabled),
      l1Reads(0),
      l1Writes(0),
      l1Misses(0),
      usefulPrefetches(0),
      uselessPrefetches(0) {
    
    // Validate stride table size
    if (config.strideTableSize <= 0) {
        throw std::invalid_argument("Stride table size must be positive");
    }
    
    // Initialize L2 cache if configured
    if (config.l2Config) {
        l2.emplace(*config.l2Config);
    }
    
    // Initialize adaptive prefetcher if enabled
    if (config.useAdaptivePrefetching) {
        adaptivePrefetcher.emplace(config.l1Config.prefetchDistance);
    }
}

void MemoryHierarchy::access(uint32_t address, bool isWrite) {
    // Update stride prediction
    if (prefetchEnabled) {
        stridePredictor.update(address);
    }
    
    // Use optional pointer for the L2 cache
    std::optional<Cache*> nextLevelPtr = l2 ? std::optional<Cache*>(&*l2) : std::nullopt;
    
    // Track if this was a prefetch hit
    bool prefetchHit = false;
    
    // Access L1 cache
    bool l1Hit = l1.access(
        address, 
        isWrite, 
        nextLevelPtr, 
        prefetchEnabled ? std::optional<StridePredictor*>(&stridePredictor) : std::nullopt
    );
    
    // Update statistics
    if (isWrite) {
        l1Writes++;
    } else {
        l1Reads++;
    }
    
    if (!l1Hit) {
        // L1 miss
        l1Misses++;
        
        // Check if L2 is configured
        if (l2) {
            static_cast<void>(l2->access(address, isWrite, std::nullopt));
        }
    } else if (prefetchHit) {
        // This was a prefetch hit
        recordPrefetchOutcome(true);
    }
    
    // Update adaptive prefetching if enabled
    if (adaptivePrefetcher && prefetchEnabled) {
        adaptivePrefetcher->updateHistory(address);
        
        // Periodically adapt prefetch distance (e.g., every 1000 accesses)
        if ((l1Reads + l1Writes) % 1000 == 0) {
            adaptPrefetchDistance();
        }
    }
}

void MemoryHierarchy::processTrace(const std::string& traceFilePath) {
    // Check if file exists
    if (!std::filesystem::exists(traceFilePath)) {
        std::cerr << "Error: Trace file " << traceFilePath << " does not exist." << std::endl;
        return;
    }
    
    // Parse and process the trace file
    TraceParser parser(traceFilePath);
    
    std::cout << "Processing trace file: " << traceFilePath << std::endl;
    int accessCount = 0;
    
    // Process each access in the trace
    while (auto memAccessOpt = parser.getNextAccess()) {
        // Use the optional value
        access(memAccessOpt->address, memAccessOpt->isWrite);
        accessCount++;
        
        // Show progress every 10,000 accesses
        if (accessCount % 10000 == 0) {
            std::cout << "Processed " << accessCount << " accesses..." << std::endl;
        }
    }
    
    std::cout << "Completed processing " << accessCount << " memory accesses." << std::endl;
}

void MemoryHierarchy::recordPrefetchOutcome(bool useful) {
    if (useful) {
        usefulPrefetches++;
    } else {
        uselessPrefetches++;
    }
    
    // Update adaptive prefetcher if enabled
    if (adaptivePrefetcher) {
        adaptivePrefetcher->recordPrefetchOutcome(useful);
    }
}

void MemoryHierarchy::adaptPrefetchDistance() {
    if (!adaptivePrefetcher) {
        return;
    }
    
    // Let the adaptive prefetcher adjust the strategy
    adaptivePrefetcher->adaptPrefetchDistance();
    
    // Update our prefetch distance
    prefetchDistance = adaptivePrefetcher->getPrefetchDistance();
}

double MemoryHierarchy::getL1MissRate() const {
    const int totalAccesses = l1Reads + l1Writes;
    return totalAccesses > 0 ? static_cast<double>(l1Misses) / totalAccesses : 0.0;
}

double MemoryHierarchy::getPrefetchAccuracy() const {
    const int totalPrefetches = usefulPrefetches + uselessPrefetches;
    return totalPrefetches > 0 ? static_cast<double>(usefulPrefetches) / totalPrefetches : 0.0;
}

std::string MemoryHierarchy::getPrefetchStats() const {
    std::stringstream ss;
    
    ss << "Prefetching Statistics:" << std::endl;
    ss << "  Useful Prefetches: " << usefulPrefetches << std::endl;
    ss << "  Useless Prefetches: " << uselessPrefetches << std::endl;
    ss << "  Prefetch Accuracy: " << std::fixed << std::setprecision(2) 
       << (getPrefetchAccuracy() * 100.0) << "%" << std::endl;
    ss << "  Current Prefetch Distance: " << prefetchDistance << std::endl;
    
    if (adaptivePrefetcher) {
        ss << "  Adaptive Prefetching: Enabled" << std::endl;
        ss << "  Current Strategy: ";
        
        switch (adaptivePrefetcher->getStrategy()) {
            case AdaptivePrefetcher::Strategy::Sequential:
                ss << "Sequential";
                break;
            case AdaptivePrefetcher::Strategy::Stride:
                ss << "Stride-based";
                break;
            case AdaptivePrefetcher::Strategy::Adaptive:
                ss << "Adaptive";
                break;
        }
        ss << std::endl;
    } else {
        ss << "  Adaptive Prefetching: Disabled" << std::endl;
    }
    
    return ss.str();
}

void MemoryHierarchy::printStats() const {
    std::cout << "Memory Hierarchy Statistics" << std::endl;
    std::cout << "===========================" << std::endl;
    
    // Print L1 cache statistics
    std::cout << "L1 Cache:" << std::endl;
    l1.printStats();
    
    // Print L2 cache statistics if present
    if (l2) {
        std::cout << std::endl << "L2 Cache:" << std::endl;
        l2->printStats();
    }
    
    // Print overall statistics
    std::cout << std::endl << "Overall Statistics:" << std::endl;
    std::cout << "  Total Memory Accesses: " << (l1Reads + l1Writes) << std::endl;
    std::cout << "  Reads: " << l1Reads << std::endl;
    std::cout << "  Writes: " << l1Writes << std::endl;
    std::cout << "  L1 Misses: " << l1Misses << std::endl;
    std::cout << "  L1 Miss Rate: " << std::fixed << std::setprecision(2) 
              << (getL1MissRate() * 100.0) << "%" << std::endl;
    
    // Print prefetching statistics if enabled
    if (prefetchEnabled) {
        std::cout << std::endl << getPrefetchStats();
        
        // Print stride predictor statistics
        std::cout << std::endl << "Stride Predictor Statistics:" << std::endl;
        stridePredictor.printStats();
    }
}

void MemoryHierarchy::resetStats() {
    l1Reads = 0;
    l1Writes = 0;
    l1Misses = 0;
    usefulPrefetches = 0;
    uselessPrefetches = 0;
    
    // Reset component statistics
    l1.resetStats();
    
    if (l2) {
        l2->resetStats();
    }
    
    if (adaptivePrefetcher) {
        adaptivePrefetcher->resetStats();
    }
    
    stridePredictor.resetStats();
}

void MemoryHierarchy::compareWith(const MemoryHierarchy& other) const {
    std::cout << "Performance Comparison" << std::endl;
    std::cout << "======================" << std::endl;
    
    // Calculate miss rate improvement
    double thisMissRate = getL1MissRate();
    double otherMissRate = other.getL1MissRate();
    double missRateImprovement = 0.0;
    
    if (otherMissRate > 0) {
        missRateImprovement = (otherMissRate - thisMissRate) / otherMissRate * 100.0;
    }
    
    // Calculate memory traffic reduction
    int thisMemTraffic = l1Misses;
    int otherMemTraffic = other.getL1Misses();
    double memTrafficReduction = 0.0;
    
    if (otherMemTraffic > 0) {
        memTrafficReduction = static_cast<double>(otherMemTraffic - thisMemTraffic) / otherMemTraffic * 100.0;
    }
    
    // Print comparison metrics
    std::cout << "Miss Rate:" << std::endl;
    std::cout << "  Configuration 1: " << std::fixed << std::setprecision(2) 
              << (thisMissRate * 100.0) << "%" << std::endl;
    std::cout << "  Configuration 2: " << std::fixed << std::setprecision(2) 
              << (otherMissRate * 100.0) << "%" << std::endl;
    std::cout << "  Improvement: " << std::fixed << std::setprecision(2) 
              << missRateImprovement << "%" << std::endl;
    
    std::cout << "Memory Traffic:" << std::endl;
    std::cout << "  Configuration 1: " << thisMemTraffic << " misses" << std::endl;
    std::cout << "  Configuration 2: " << otherMemTraffic << " misses" << std::endl;
    std::cout << "  Reduction: " << std::fixed << std::setprecision(2) 
              << memTrafficReduction << "%" << std::endl;
    
    // Compare prefetching if enabled in both
    if (prefetchEnabled && other.prefetchEnabled) {
        double thisAccuracy = getPrefetchAccuracy();
        double otherAccuracy = other.getPrefetchAccuracy();
        
        std::cout << "Prefetch Accuracy:" << std::endl;
        std::cout << "  Configuration 1: " << std::fixed << std::setprecision(2) 
                  << (thisAccuracy * 100.0) << "%" << std::endl;
        std::cout << "  Configuration 2: " << std::fixed << std::setprecision(2) 
                  << (otherAccuracy * 100.0) << "%" << std::endl;
    }
}

double MemoryHierarchy::getL2HitRate() const {
    if (!l2) {
        return 0.0;
    }
    return l2->getHitRatio();
}

double MemoryHierarchy::getL2MissRate() const {
    if (!l2) {
        return 0.0;
    }
    return l2->getMissRatio();
}

} // namespace cachesim