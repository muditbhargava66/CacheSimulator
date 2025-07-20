/**
 * @file processor_core.h
 * @brief Processor core model for multi-processor simulation
 * @author Mudit Bhargava
 * @date 2025-05-31
 * @version 1.2.0
 *
 * This file implements a processor core model that includes private L1 cache
 * and participates in cache coherence protocols.
 */

#pragma once

#include <memory>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "../cache.h"
#include "../memory_hierarchy.h"
#include "coherence_controller.h"

namespace cachesim {

// Forward declarations
class CoherenceController;
class InterconnectInterface;

/**
 * @enum ProcessorState
 * @brief States of a processor core
 */
enum class ProcessorState {
    Running,     ///< Actively processing
    Stalled,     ///< Stalled waiting for memory/coherence
    Idle,        ///< No work to do
    Synchronizing ///< Waiting at synchronization point
};

/**
 * @struct MemoryRequest
 * @brief Represents a memory request from a processor
 */
struct MemoryRequest {
    uint32_t address;
    bool isWrite;
    uint64_t timestamp;
    uint32_t processorId;
    bool isAtomic;      ///< Atomic operation (for synchronization)
    bool isAcquire;     ///< Memory acquire semantics
    bool isRelease;     ///< Memory release semantics
};

/**
 * @class ProcessorCore
 * @brief Models a single processor core with private L1 cache
 */
class ProcessorCore {
public:
    /**
     * @brief Constructor
     * @param id Unique processor ID
     * @param l1Config Configuration for L1 cache
     * @param coherenceController Shared coherence controller
     */
    ProcessorCore(uint32_t id, const CacheConfig& l1Config,
                  std::shared_ptr<CoherenceController> coherenceController);
    
    /**
     * @brief Process a memory access
     * @param address Memory address
     * @param isWrite Write operation flag
     * @return Latency in cycles
     */
    uint32_t access(uint32_t address, bool isWrite);
    
    /**
     * @brief Process an atomic memory operation
     * @param address Memory address
     * @param operation Atomic operation type
     * @return Latency in cycles
     */
    uint32_t atomicAccess(uint32_t address, bool isWrite);
    
    /**
     * @brief Handle coherence request from another processor
     * @param request Coherence request
     * @return Response latency
     */
    uint32_t handleCoherenceRequest(const CoherenceRequest& request);
    
    /**
     * @brief Execute a memory barrier
     * @param isAcquire Acquire barrier
     * @param isRelease Release barrier
     */
    void memoryBarrier(bool isAcquire, bool isRelease);
    
    /**
     * @brief Get processor state
     */
    [[nodiscard]] ProcessorState getState() const { return state_; }
    
    /**
     * @brief Get processor ID
     */
    [[nodiscard]] uint32_t getId() const { return processorId_; }
    
    /**
     * @brief Get L1 cache reference
     */
    [[nodiscard]] Cache& getL1Cache() { return l1Cache_; }
    [[nodiscard]] const Cache& getL1Cache() const { return l1Cache_; }
    
    /**
     * @brief Get processor statistics
     */
    struct ProcessorStats {
        uint64_t totalAccesses;
        uint64_t l1Hits;
        uint64_t l1Misses;
        uint64_t coherenceMisses;
        uint64_t atomicOperations;
        uint64_t barriers;
        uint64_t stalledCycles;
        double avgLatency;
    };
    
    [[nodiscard]] ProcessorStats getStats() const;
    void resetStats();
    void printStats() const;
    
private:
    uint32_t processorId_;
    ProcessorState state_;
    Cache l1Cache_;
    std::shared_ptr<CoherenceController> coherenceController_;
    
    // Request queue for outstanding memory operations
    std::queue<MemoryRequest> requestQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    
    // Statistics
    std::atomic<uint64_t> totalAccesses_{0};
    std::atomic<uint64_t> l1Hits_{0};
    std::atomic<uint64_t> l1Misses_{0};
    std::atomic<uint64_t> coherenceMisses_{0};
    std::atomic<uint64_t> atomicOperations_{0};
    std::atomic<uint64_t> barriers_{0};
    std::atomic<uint64_t> stalledCycles_{0};
    std::atomic<uint64_t> totalLatency_{0};
    
    // Memory ordering tracking
    std::vector<uint32_t> pendingReleases_;
    std::mutex orderingMutex_;
    
    // Helper methods
    uint32_t handleL1Miss(uint32_t address, bool isWrite);
    void updateCoherenceState(uint32_t address, MESIState newState);
    bool checkCoherencePermission(uint32_t address, bool isWrite);
    void drainPendingRequests();
};

/**
 * @class MultiProcessorSystem
 * @brief Manages multiple processor cores and shared resources
 */
class MultiProcessorSystem {
public:
    /**
     * @brief Configuration for multi-processor system
     */
    struct Config {
        uint32_t numProcessors;
        CacheConfig l1Config;
        std::optional<CacheConfig> sharedL2Config;
        std::optional<CacheConfig> sharedL3Config;
        bool enableCoherence;
        uint32_t interconnectLatency;
        uint32_t memoryLatency;
    };
    
    /**
     * @brief Constructor
     * @param config System configuration
     */
    explicit MultiProcessorSystem(const Config& config);
    
    /**
     * @brief Get processor by ID
     */
    [[nodiscard]] ProcessorCore* getProcessor(uint32_t id);
    [[nodiscard]] const ProcessorCore* getProcessor(uint32_t id) const;
    
    /**
     * @brief Process memory access from a specific processor
     */
    uint32_t processAccess(uint32_t processorId, uint32_t address, bool isWrite);
    
    /**
     * @brief Process atomic operation from a specific processor
     */
    uint32_t processAtomicAccess(uint32_t processorId, uint32_t address, bool isWrite);
    
    /**
     * @brief Execute barrier on a processor
     */
    void executeBarrier(uint32_t processorId, bool isAcquire, bool isRelease);
    
    /**
     * @brief Global synchronization barrier (all processors)
     */
    void globalBarrier();
    
    /**
     * @brief Get system-wide statistics
     */
    struct SystemStats {
        uint64_t totalAccesses;
        uint64_t totalHits;
        uint64_t totalMisses;
        uint64_t coherenceTraffic;
        uint64_t interconnectMessages;
        double avgLatency;
        double systemThroughput;
    };
    
    [[nodiscard]] SystemStats getSystemStats() const;
    void printSystemStats() const;
    void resetAllStats();
    
    /**
     * @brief Simulate parallel trace execution
     * @param traces Vector of trace files (one per processor)
     * @return Total simulation time
     */
    uint64_t simulateParallelTraces(const std::vector<std::string>& traces);
    
private:
    Config config_;
    std::vector<std::unique_ptr<ProcessorCore>> processors_;
    std::shared_ptr<CoherenceController> coherenceController_;
    std::unique_ptr<Cache> sharedL2_;
    std::unique_ptr<Cache> sharedL3_;
    
    // Synchronization
    std::atomic<uint32_t> barrierCount_{0};
    std::mutex barrierMutex_;
    std::condition_variable barrierCV_;
    
    // System-wide statistics
    std::atomic<uint64_t> systemCycles_{0};
    std::atomic<uint64_t> coherenceMessages_{0};
    
    // Helper methods
    void initializeSystem();
    uint32_t routeToSharedCache(uint32_t address, bool isWrite);
};

} // namespace cachesim
