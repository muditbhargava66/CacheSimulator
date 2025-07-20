/**
 * @file interconnect.cpp
 * @brief Implementation of interconnect models
 * @author Mudit Bhargava
 * @date 2025-05-31
 * @version 1.2.0
 */

#include "interconnect.h"
#include <algorithm>
#include <chrono>
#include <thread>

namespace cachesim {

// BusInterconnect Implementation

BusInterconnect::BusInterconnect(uint32_t numProcessors, uint32_t busLatency, uint32_t busWidth)
    : numProcessors_(numProcessors), busLatency_(busLatency), busWidth_(busWidth),
      messageQueues_(numProcessors), queueMutexes_(numProcessors) {
}

uint32_t BusInterconnect::sendMessage(const InterconnectMessage& message) {
    // Bus arbitration
    std::unique_lock<std::mutex> busLock(busMutex_);
    
    // Wait if bus is occupied
    while (busOccupied_) {
        congestionEvents_++;
        busCV_.wait(busLock);
    }
    
    busOccupied_ = true;
    busLock.unlock();
    
    // Calculate transmission time based on message size
    uint32_t transmissionCycles = busLatency_ + (message.payload.size() / busWidth_);
    
    // Simulate transmission delay
    std::this_thread::sleep_for(std::chrono::nanoseconds(transmissionCycles));
    
    // Deliver message to destination
    if (message.destId < numProcessors_) {
        std::lock_guard<std::mutex> queueLock(queueMutexes_[message.destId]);
        messageQueues_[message.destId].push(message);
    }
    
    // Update statistics
    totalMessages_++;
    totalLatency_ += transmissionCycles;
    busUtilizationCycles_ += transmissionCycles;
    totalCycles_ += transmissionCycles;
    
    // Release bus
    busLock.lock();
    busOccupied_ = false;
    busCV_.notify_one();
    
    return transmissionCycles;
}

bool BusInterconnect::hasMessages(uint32_t processorId) const {
    if (processorId >= numProcessors_) return false;
    
    std::lock_guard<std::mutex> lock(queueMutexes_[processorId]);
    return !messageQueues_[processorId].empty();
}

std::optional<InterconnectMessage> BusInterconnect::receiveMessage(uint32_t processorId) {
    if (processorId >= numProcessors_) return std::nullopt;
    
    std::lock_guard<std::mutex> lock(queueMutexes_[processorId]);
    if (messageQueues_[processorId].empty()) {
        return std::nullopt;
    }
    
    InterconnectMessage message = messageQueues_[processorId].front();
    messageQueues_[processorId].pop();
    return message;
}

BusInterconnect::InterconnectStats BusInterconnect::getStats() const {
    InterconnectStats stats;
    stats.totalMessages = totalMessages_;
    stats.totalLatency = totalLatency_;
    stats.congestionEvents = congestionEvents_;
    stats.avgHopCount = 1.0; // Bus is always 1 hop
    stats.utilization = totalCycles_ > 0 ? 
        static_cast<double>(busUtilizationCycles_) / totalCycles_ : 0.0;
    return stats;
}

void BusInterconnect::resetStats() {
    totalMessages_ = 0;
    totalLatency_ = 0;
    congestionEvents_ = 0;
    busUtilizationCycles_ = 0;
    totalCycles_ = 0;
}

// MeshInterconnect Implementation

MeshInterconnect::MeshInterconnect(uint32_t numProcessors, uint32_t linkLatency, uint32_t meshWidth)
    : numProcessors_(numProcessors), linkLatency_(linkLatency), meshWidth_(meshWidth),
      meshHeight_((numProcessors + meshWidth - 1) / meshWidth),
      routers_(numProcessors) {
}

uint32_t MeshInterconnect::sendMessage(const InterconnectMessage& message) {
    // Calculate route
    auto route = calculateRoute(message.sourceId, message.destId);
    
    if (route.empty()) {
        return 0; // Same source and destination
    }
    
    // Forward message through routers
    uint32_t totalLatency = 0;
    InterconnectMessage msg = message;
    
    for (size_t i = 0; i < route.size() - 1; ++i) {
        uint32_t currentNode = route[i];
        uint32_t nextNode = route[i + 1];
        
        // Check congestion at current router
        if (routers_[currentNode].congestionLevel > 10) {
            congestionEvents_++;
            totalLatency += linkLatency_ * 2; // Congestion penalty
        } else {
            totalLatency += linkLatency_;
        }
        
        // Update congestion level
        routers_[currentNode].congestionLevel++;
        
        // Place in next router's queue
        {
            std::lock_guard<std::mutex> lock(routers_[nextNode].mutex);
            routers_[nextNode].inputQueue.push(msg);
        }
        
        msg.hopCount++;
    }
    
    // Deliver to final destination
    {
        std::lock_guard<std::mutex> lock(routers_[message.destId].mutex);
        routers_[message.destId].outputQueue.push(msg);
    }
    
    // Update statistics
    totalMessages_++;
    totalHops_ += msg.hopCount;
    
    return totalLatency;
}

bool MeshInterconnect::hasMessages(uint32_t processorId) const {
    if (processorId >= numProcessors_) return false;
    
    std::lock_guard<std::mutex> lock(routers_[processorId].mutex);
    return !routers_[processorId].outputQueue.empty();
}

std::optional<InterconnectMessage> MeshInterconnect::receiveMessage(uint32_t processorId) {
    if (processorId >= numProcessors_) return std::nullopt;
    
    std::lock_guard<std::mutex> lock(routers_[processorId].mutex);
    if (routers_[processorId].outputQueue.empty()) {
        return std::nullopt;
    }
    
    InterconnectMessage message = routers_[processorId].outputQueue.front();
    routers_[processorId].outputQueue.pop();
    
    // Decrease congestion level
    if (routers_[processorId].congestionLevel > 0) {
        routers_[processorId].congestionLevel--;
    }
    
    return message;
}

MeshInterconnect::InterconnectStats MeshInterconnect::getStats() const {
    InterconnectStats stats;
    stats.totalMessages = totalMessages_;
    stats.totalLatency = totalMessages_ * linkLatency_; // Simplified
    stats.congestionEvents = congestionEvents_;
    stats.avgHopCount = totalMessages_ > 0 ? 
        static_cast<double>(totalHops_) / totalMessages_ : 0.0;
    stats.utilization = 0.0; // Would need more complex tracking
    return stats;
}

void MeshInterconnect::resetStats() {
    totalMessages_ = 0;
    totalHops_ = 0;
    congestionEvents_ = 0;
}

std::pair<uint32_t, uint32_t> MeshInterconnect::idToCoordinates(uint32_t id) const {
    return {id % meshWidth_, id / meshWidth_};
}

uint32_t MeshInterconnect::coordinatesToId(uint32_t x, uint32_t y) const {
    return y * meshWidth_ + x;
}

std::vector<uint32_t> MeshInterconnect::calculateRoute(uint32_t source, uint32_t dest) const {
    if (source == dest) {
        return {source};
    }
    
    // XY routing algorithm
    auto [sx, sy] = idToCoordinates(source);
    auto [dx, dy] = idToCoordinates(dest);
    
    std::vector<uint32_t> route;
    route.push_back(source);
    
    // Route in X dimension first
    while (sx != dx) {
        if (sx < dx) {
            sx++;
        } else {
            sx--;
        }
        route.push_back(coordinatesToId(sx, sy));
    }
    
    // Then route in Y dimension
    while (sy != dy) {
        if (sy < dy) {
            sy++;
        } else {
            sy--;
        }
        route.push_back(coordinatesToId(sx, sy));
    }
    
    return route;
}

// CrossbarInterconnect Implementation

CrossbarInterconnect::CrossbarInterconnect(uint32_t numProcessors, uint32_t crossbarLatency)
    : numProcessors_(numProcessors), crossbarLatency_(crossbarLatency),
      connections_(numProcessors, std::vector<std::queue<InterconnectMessage>>(numProcessors)),
      portOccupied_(numProcessors) {
}

uint32_t CrossbarInterconnect::sendMessage(const InterconnectMessage& message) {
    if (message.sourceId >= numProcessors_ || message.destId >= numProcessors_) {
        return 0;
    }
    
    // Check if destination port is occupied
    bool expected = false;
    if (!portOccupied_[message.destId].compare_exchange_strong(expected, true)) {
        portConflicts_++;
        // Wait for port to be free (simplified)
        std::this_thread::sleep_for(std::chrono::nanoseconds(crossbarLatency_));
    }
    
    // Send message through crossbar
    {
        std::lock_guard<std::mutex> lock(connectionMutex_);
        connections_[message.sourceId][message.destId].push(message);
    }
    
    // Update statistics
    totalMessages_++;
    totalLatency_ += crossbarLatency_;
    
    // Release port
    portOccupied_[message.destId] = false;
    
    return crossbarLatency_;
}

bool CrossbarInterconnect::hasMessages(uint32_t processorId) const {
    if (processorId >= numProcessors_) return false;
    
    // Check all incoming connections
    std::lock_guard<std::mutex> lock(connectionMutex_);
    for (uint32_t src = 0; src < numProcessors_; ++src) {
        if (!connections_[src][processorId].empty()) {
            return true;
        }
    }
    
    return false;
}

std::optional<InterconnectMessage> CrossbarInterconnect::receiveMessage(uint32_t processorId) {
    if (processorId >= numProcessors_) return std::nullopt;
    
    // Check all incoming connections (round-robin)
    std::lock_guard<std::mutex> lock(connectionMutex_);
    for (uint32_t src = 0; src < numProcessors_; ++src) {
        if (!connections_[src][processorId].empty()) {
            InterconnectMessage message = connections_[src][processorId].front();
            connections_[src][processorId].pop();
            return message;
        }
    }
    
    return std::nullopt;
}

CrossbarInterconnect::InterconnectStats CrossbarInterconnect::getStats() const {
    InterconnectStats stats;
    stats.totalMessages = totalMessages_;
    stats.totalLatency = totalLatency_;
    stats.congestionEvents = portConflicts_;
    stats.avgHopCount = 1.0; // Crossbar is always 1 hop
    stats.utilization = 0.0; // Would need more complex tracking
    return stats;
}

void CrossbarInterconnect::resetStats() {
    totalMessages_ = 0;
    totalLatency_ = 0;
    portConflicts_ = 0;
}

} // namespace cachesim
