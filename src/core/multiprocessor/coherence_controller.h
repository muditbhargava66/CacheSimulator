/**
 * @file coherence_controller.h
 * @brief Cache coherence controller for multi-processor systems
 * @author Mudit Bhargava
 * @date 2025-05-31
 * @version 1.2.0
 *
 * This file implements a directory-based cache coherence controller
 * that manages the MESI protocol across multiple processors.
 */

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include "../mesi_protocol.h"

namespace cachesim {

// Forward declarations
class ProcessorCore;

/**
 * @enum CoherenceRequestType
 * @brief Types of coherence requests
 */
enum class CoherenceRequestType {
    ReadRequest,        ///< Processor wants to read
    WriteRequest,       ///< Processor wants to write
    InvalidateRequest,  ///< Invalidate copies in other caches
    WritebackRequest,   ///< Writing back dirty data
    ShareRequest,       ///< Downgrade from exclusive to shared
    FlushRequest        ///< Flush cache line
};

/**
 * @struct CoherenceRequest
 * @brief Represents a coherence request between processors
 */
struct CoherenceRequest {
    uint32_t requestorId;
    uint32_t address;
    CoherenceRequestType type;
    MESIState currentState;
    bool needsData;
    uint64_t timestamp;
};

/**
 * @struct CoherenceResponse
 * @brief Response to a coherence request
 */
struct CoherenceResponse {
    bool granted;
    MESIState newState;
    bool hasData;
    std::vector<uint8_t> data;
    uint32_t latency;
    std::vector<uint32_t> invalidatedProcessors;
};

/**
 * @class DirectoryEntry
 * @brief Directory entry tracking cache line ownership
 */
class DirectoryEntry {
public:
    DirectoryEntry() : state_(MESIState::Invalid), owner_(-1), dirty_(false) {}
    
    MESIState state_;
    int32_t owner_;                      ///< Exclusive/Modified owner (-1 if none)
    std::unordered_set<uint32_t> sharers_; ///< Set of processors sharing the line
    bool dirty_;                         ///< Line has been modified
    uint64_t lastAccess_;                ///< Last access timestamp
};

/**
 * @class CoherenceController
 * @brief Manages cache coherence across multiple processors
 */
class CoherenceController {
public:
    /**
     * @brief Constructor
     * @param numProcessors Number of processors in the system
     * @param interconnectLatency Latency for inter-processor communication
     */
    CoherenceController(uint32_t numProcessors, uint32_t interconnectLatency);
    
    /**
     * @brief Process a coherence request
     * @param request The coherence request
     * @return Response with action to take
     */
    CoherenceResponse processRequest(const CoherenceRequest& request);
    
    /**
     * @brief Register a processor's cache state change
     * @param processorId Processor ID
     * @param address Memory address
     * @param oldState Previous MESI state
     * @param newState New MESI state
     */
    void updateCacheState(uint32_t processorId, uint32_t address,
                          MESIState oldState, MESIState newState);
    
    /**
     * @brief Handle writeback from a processor
     * @param processorId Processor ID
     * @param address Memory address
     * @param data Data to write back
     */
    void handleWriteback(uint32_t processorId, uint32_t address,
                        const std::vector<uint8_t>& data);
    
    /**
     * @brief Check if a processor can perform an operation
     * @param processorId Processor ID
     * @param address Memory address
     * @param isWrite Whether it's a write operation
     * @return True if operation is allowed
     */
    bool checkPermission(uint32_t processorId, uint32_t address, bool isWrite);
    
    /**
     * @brief Get current state of a cache line for a processor
     */
    [[nodiscard]] MESIState getCacheLineState(uint32_t processorId, uint32_t address) const;
    
    /**
     * @brief Statistics tracking
     */
    struct CoherenceStats {
        uint64_t readRequests;
        uint64_t writeRequests;
        uint64_t invalidations;
        uint64_t writebacks;
        uint64_t stateTransitions;
        uint64_t coherenceMessages;
        std::unordered_map<std::string, uint64_t> transitionCounts;
    };
    
    [[nodiscard]] CoherenceStats getStats() const;
    void resetStats();
    void printStats() const;
    
    /**
     * @brief Set processor references for callbacks
     */
    void setProcessors(std::vector<ProcessorCore*> processors) {
        processors_ = processors;
    }
    
private:
    uint32_t numProcessors_;
    uint32_t interconnectLatency_;
    
    // Directory tracking cache line states
    mutable std::mutex directoryMutex_;
    std::unordered_map<uint32_t, DirectoryEntry> directory_;
    
    // Processor references for callbacks
    std::vector<ProcessorCore*> processors_;
    
    // Statistics
    std::atomic<uint64_t> readRequests_{0};
    std::atomic<uint64_t> writeRequests_{0};
    std::atomic<uint64_t> invalidations_{0};
    std::atomic<uint64_t> writebacks_{0};
    std::atomic<uint64_t> stateTransitions_{0};
    std::atomic<uint64_t> coherenceMessages_{0};
    
    // Helper methods
    DirectoryEntry& getOrCreateEntry(uint32_t address);
    std::vector<uint32_t> invalidateSharers(uint32_t address, uint32_t excludeProcessor);
    void broadcastInvalidate(uint32_t address, const std::vector<uint32_t>& processors);
    uint32_t calculateLatency(const CoherenceRequest& request, 
                             const std::vector<uint32_t>& involvedProcessors);
    std::string getTransitionKey(MESIState from, MESIState to) const;
};

/**
 * @class SnoopingController
 * @brief Alternative snooping-based coherence controller
 */
class SnoopingController : public CoherenceController {
public:
    SnoopingController(uint32_t numProcessors, uint32_t busLatency);
    
    /**
     * @brief Broadcast snoop request on the bus
     */
    CoherenceResponse broadcastSnoop(const CoherenceRequest& request);
    
    /**
     * @brief Handle snoop response from a processor
     */
    void handleSnoopResponse(uint32_t processorId, const CoherenceRequest& request,
                           bool hasData, MESIState state);
    
private:
    uint32_t busLatency_;
    std::atomic<bool> busLocked_{false};
    
    // Snoop responses tracking
    struct SnoopTransaction {
        CoherenceRequest request;
        std::unordered_map<uint32_t, MESIState> responses;
        std::unordered_set<uint32_t> dataProviders;
        std::mutex mutex;
    };
    
    std::unordered_map<uint64_t, SnoopTransaction> activeTransactions_;
    std::atomic<uint64_t> transactionId_{0};
};

} // namespace cachesim
