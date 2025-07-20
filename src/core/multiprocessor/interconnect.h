/**
 * @file interconnect.h
 * @brief Interconnect model for multi-processor communication
 * @author Mudit Bhargava
 * @date 2025-05-31
 * @version 1.2.0
 *
 * This file implements various interconnect topologies for
 * multi-processor cache coherence communication.
 */

#pragma once

#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace cachesim {

/**
 * @enum InterconnectType
 * @brief Types of interconnect topologies
 */
enum class InterconnectType {
    Bus,           ///< Shared bus (simple, limited scalability)
    Crossbar,      ///< Full crossbar (high bandwidth, expensive)
    Mesh,          ///< 2D mesh network
    Ring,          ///< Ring network
    Torus          ///< 2D torus (mesh with wraparound)
};

/**
 * @struct InterconnectMessage
 * @brief Message passed through the interconnect
 */
struct InterconnectMessage {
    uint32_t sourceId;
    uint32_t destId;
    uint32_t address;
    enum Type {
        CoherenceRequest,
        CoherenceResponse,
        DataTransfer,
        Acknowledgment
    } type;
    std::vector<uint8_t> payload;
    uint64_t timestamp;
    uint32_t hopCount;
};

/**
 * @class InterconnectInterface
 * @brief Abstract interface for interconnect implementations
 */
class InterconnectInterface {
public:
    virtual ~InterconnectInterface() = default;
    
    /**
     * @brief Send a message through the interconnect
     * @param message Message to send
     * @return Latency in cycles
     */
    virtual uint32_t sendMessage(const InterconnectMessage& message) = 0;
    
    /**
     * @brief Check if there are pending messages for a processor
     * @param processorId Processor ID
     * @return True if messages are available
     */
    virtual bool hasMessages(uint32_t processorId) const = 0;
    
    /**
     * @brief Receive a message for a processor
     * @param processorId Processor ID
     * @return The message (nullopt if none available)
     */
    virtual std::optional<InterconnectMessage> receiveMessage(uint32_t processorId) = 0;
    
    /**
     * @brief Get interconnect statistics
     */
    struct InterconnectStats {
        uint64_t totalMessages;
        uint64_t totalLatency;
        uint64_t congestionEvents;
        double avgHopCount;
        double utilization;
    };
    
    virtual InterconnectStats getStats() const = 0;
    virtual void resetStats() = 0;
};

/**
 * @class BusInterconnect
 * @brief Simple shared bus interconnect
 */
class BusInterconnect : public InterconnectInterface {
public:
    BusInterconnect(uint32_t numProcessors, uint32_t busLatency, uint32_t busWidth);
    
    uint32_t sendMessage(const InterconnectMessage& message) override;
    bool hasMessages(uint32_t processorId) const override;
    std::optional<InterconnectMessage> receiveMessage(uint32_t processorId) override;
    InterconnectStats getStats() const override;
    void resetStats() override;
    
private:
    uint32_t numProcessors_;
    uint32_t busLatency_;
    uint32_t busWidth_;
    
    // Message queues per processor
    std::vector<std::queue<InterconnectMessage>> messageQueues_;
    mutable std::vector<std::mutex> queueMutexes_;
    
    // Bus arbitration
    std::mutex busMutex_;
    std::condition_variable busCV_;
    std::atomic<bool> busOccupied_{false};
    
    // Statistics
    std::atomic<uint64_t> totalMessages_{0};
    std::atomic<uint64_t> totalLatency_{0};
    std::atomic<uint64_t> congestionEvents_{0};
    std::atomic<uint64_t> busUtilizationCycles_{0};
    std::atomic<uint64_t> totalCycles_{0};
};

/**
 * @class MeshInterconnect
 * @brief 2D mesh network interconnect
 */
class MeshInterconnect : public InterconnectInterface {
public:
    MeshInterconnect(uint32_t numProcessors, uint32_t linkLatency, uint32_t meshWidth);
    
    uint32_t sendMessage(const InterconnectMessage& message) override;
    bool hasMessages(uint32_t processorId) const override;
    std::optional<InterconnectMessage> receiveMessage(uint32_t processorId) override;
    InterconnectStats getStats() const override;
    void resetStats() override;
    
private:
    uint32_t numProcessors_;
    uint32_t linkLatency_;
    uint32_t meshWidth_;
    uint32_t meshHeight_;
    
    // Convert processor ID to mesh coordinates
    std::pair<uint32_t, uint32_t> idToCoordinates(uint32_t id) const;
    uint32_t coordinatesToId(uint32_t x, uint32_t y) const;
    
    // Calculate shortest path between nodes
    std::vector<uint32_t> calculateRoute(uint32_t source, uint32_t dest) const;
    
    // Router queues at each node
    struct Router {
        std::queue<InterconnectMessage> inputQueue;
        std::queue<InterconnectMessage> outputQueue;
        mutable std::mutex mutex;
        std::atomic<uint32_t> congestionLevel{0};
    };
    
    std::vector<Router> routers_;
    
    // Statistics
    std::atomic<uint64_t> totalMessages_{0};
    std::atomic<uint64_t> totalHops_{0};
    std::atomic<uint64_t> congestionEvents_{0};
};

/**
 * @class CrossbarInterconnect
 * @brief Full crossbar interconnect (all-to-all connections)
 */
class CrossbarInterconnect : public InterconnectInterface {
public:
    CrossbarInterconnect(uint32_t numProcessors, uint32_t crossbarLatency);
    
    uint32_t sendMessage(const InterconnectMessage& message) override;
    bool hasMessages(uint32_t processorId) const override;
    std::optional<InterconnectMessage> receiveMessage(uint32_t processorId) override;
    InterconnectStats getStats() const override;
    void resetStats() override;
    
private:
    uint32_t numProcessors_;
    uint32_t crossbarLatency_;
    
    // Point-to-point connections
    std::vector<std::vector<std::queue<InterconnectMessage>>> connections_;
    mutable std::mutex connectionMutex_;
    
    // Crossbar arbitration
    std::vector<std::atomic<bool>> portOccupied_;
    
    // Statistics
    std::atomic<uint64_t> totalMessages_{0};
    std::atomic<uint64_t> totalLatency_{0};
    std::atomic<uint64_t> portConflicts_{0};
};

/**
 * @class InterconnectFactory
 * @brief Factory for creating interconnect instances
 */
class InterconnectFactory {
public:
    static std::unique_ptr<InterconnectInterface> create(
        InterconnectType type,
        uint32_t numProcessors,
        uint32_t baseLatency) {
        
        switch (type) {
            case InterconnectType::Bus:
                return std::make_unique<BusInterconnect>(numProcessors, baseLatency, 64);
                
            case InterconnectType::Crossbar:
                return std::make_unique<CrossbarInterconnect>(numProcessors, baseLatency);
                
            case InterconnectType::Mesh:
            {
                // Calculate mesh dimensions
                uint32_t width = static_cast<uint32_t>(std::sqrt(numProcessors));
                if (width * width < numProcessors) width++;
                return std::make_unique<MeshInterconnect>(numProcessors, baseLatency, width);
            }
                
            case InterconnectType::Ring:
            case InterconnectType::Torus:
                // TODO: Implement ring and torus topologies
                return std::make_unique<BusInterconnect>(numProcessors, baseLatency, 64);
                
            default:
                return std::make_unique<BusInterconnect>(numProcessors, baseLatency, 64);
        }
    }
};

} // namespace cachesim
