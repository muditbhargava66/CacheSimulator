/**
 * @file interconnect.h
 * @brief Interconnect model for multi-processor communication
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 *
 * This file implements various interconnect topologies for
 * multi-processor cache coherence communication.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <vector>

namespace cachesim {

/**
 * @enum InterconnectType
 * @brief Types of interconnect topologies
 */
enum class InterconnectType {
  Bus,      ///< Shared bus (simple, limited scalability)
  Crossbar, ///< Full crossbar (high bandwidth, expensive)
  Mesh,     ///< 2D mesh network
  Ring,     ///< Ring network
  Torus     ///< 2D torus (mesh with wraparound)
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
  virtual uint32_t sendMessage(const InterconnectMessage &message) = 0;

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
  virtual std::optional<InterconnectMessage>
  receiveMessage(uint32_t processorId) = 0;

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
  BusInterconnect(uint32_t numProcessors, uint32_t busLatency,
                  uint32_t busWidth);

  uint32_t sendMessage(const InterconnectMessage &message) override;
  bool hasMessages(uint32_t processorId) const override;
  std::optional<InterconnectMessage>
  receiveMessage(uint32_t processorId) override;
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
  MeshInterconnect(uint32_t numProcessors, uint32_t linkLatency,
                   uint32_t meshWidth);

  uint32_t sendMessage(const InterconnectMessage &message) override;
  bool hasMessages(uint32_t processorId) const override;
  std::optional<InterconnectMessage>
  receiveMessage(uint32_t processorId) override;
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

  uint32_t sendMessage(const InterconnectMessage &message) override;
  bool hasMessages(uint32_t processorId) const override;
  std::optional<InterconnectMessage>
  receiveMessage(uint32_t processorId) override;
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
 * @class RingInterconnect
 * @brief Ring topology interconnect
 *
 * Latency model: min(clockwise hops, counter-clockwise hops) * hopLatency
 * Bidirectional ring for shortest path routing.
 */
class RingInterconnect : public InterconnectInterface {
public:
  RingInterconnect(uint32_t numProcessors, uint32_t hopLatency)
      : numProcessors_(numProcessors), hopLatency_(hopLatency),
        routers_(numProcessors) {}

  uint32_t sendMessage(const InterconnectMessage &message) override {
    uint32_t hops = calculateHops(message.sourceId, message.destId);
    uint32_t latency = hops * hopLatency_;

    // Deliver to destination
    if (message.destId < numProcessors_) {
      std::lock_guard<std::mutex> lock(routers_[message.destId].mutex);
      InterconnectMessage msg = message;
      msg.hopCount = hops;
      routers_[message.destId].queue.push(msg);
    }

    totalMessages_.fetch_add(1);
    totalHops_.fetch_add(hops);
    return latency;
  }

  bool hasMessages(uint32_t processorId) const override {
    if (processorId >= numProcessors_)
      return false;
    std::lock_guard<std::mutex> lock(routers_[processorId].mutex);
    return !routers_[processorId].queue.empty();
  }

  std::optional<InterconnectMessage>
  receiveMessage(uint32_t processorId) override {
    if (processorId >= numProcessors_)
      return std::nullopt;
    std::lock_guard<std::mutex> lock(routers_[processorId].mutex);
    if (routers_[processorId].queue.empty())
      return std::nullopt;
    auto msg = routers_[processorId].queue.front();
    routers_[processorId].queue.pop();
    return msg;
  }

  InterconnectStats getStats() const override {
    uint64_t msgs = totalMessages_.load();
    return {msgs, totalHops_.load() * hopLatency_, 0,
            msgs > 0 ? static_cast<double>(totalHops_.load()) / msgs : 0.0,
            0.0};
  }

  void resetStats() override {
    totalMessages_ = 0;
    totalHops_ = 0;
  }

private:
  uint32_t numProcessors_;
  uint32_t hopLatency_;

  uint32_t calculateHops(uint32_t src, uint32_t dst) const {
    if (src == dst)
      return 0;
    uint32_t clockwise =
        (dst >= src) ? (dst - src) : (numProcessors_ - src + dst);
    uint32_t counterClockwise = numProcessors_ - clockwise;
    return std::min(clockwise, counterClockwise);
  }

  struct RouterNode {
    std::queue<InterconnectMessage> queue;
    mutable std::mutex mutex;
  };

  std::vector<RouterNode> routers_;
  std::atomic<uint64_t> totalMessages_{0};
  std::atomic<uint64_t> totalHops_{0};
};

/**
 * @class TorusInterconnect
 * @brief 2D Torus topology with wrap-around connections
 *
 * Latency model: Manhattan distance with wrap-around * hopLatency
 * More scalable than mesh for large processor counts.
 */
class TorusInterconnect : public InterconnectInterface {
public:
  TorusInterconnect(uint32_t numProcessors, uint32_t hopLatency)
      : numProcessors_(numProcessors), hopLatency_(hopLatency),
        width_(static_cast<uint32_t>(std::ceil(std::sqrt(numProcessors)))),
        routers_(numProcessors) {
    height_ = (numProcessors + width_ - 1) / width_;
  }

  uint32_t sendMessage(const InterconnectMessage &message) override {
    uint32_t hops = calculateHops(message.sourceId, message.destId);
    uint32_t latency = hops * hopLatency_;

    if (message.destId < numProcessors_) {
      std::lock_guard<std::mutex> lock(routers_[message.destId].mutex);
      InterconnectMessage msg = message;
      msg.hopCount = hops;
      routers_[message.destId].queue.push(msg);
    }

    totalMessages_.fetch_add(1);
    totalHops_.fetch_add(hops);
    return latency;
  }

  bool hasMessages(uint32_t processorId) const override {
    if (processorId >= numProcessors_)
      return false;
    std::lock_guard<std::mutex> lock(routers_[processorId].mutex);
    return !routers_[processorId].queue.empty();
  }

  std::optional<InterconnectMessage>
  receiveMessage(uint32_t processorId) override {
    if (processorId >= numProcessors_)
      return std::nullopt;
    std::lock_guard<std::mutex> lock(routers_[processorId].mutex);
    if (routers_[processorId].queue.empty())
      return std::nullopt;
    auto msg = routers_[processorId].queue.front();
    routers_[processorId].queue.pop();
    return msg;
  }

  InterconnectStats getStats() const override {
    uint64_t msgs = totalMessages_.load();
    return {msgs, totalHops_.load() * hopLatency_, 0,
            msgs > 0 ? static_cast<double>(totalHops_.load()) / msgs : 0.0,
            0.0};
  }

  void resetStats() override {
    totalMessages_ = 0;
    totalHops_ = 0;
  }

private:
  uint32_t numProcessors_;
  uint32_t hopLatency_;
  uint32_t width_;
  uint32_t height_;

  uint32_t calculateHops(uint32_t src, uint32_t dst) const {
    if (src == dst)
      return 0;

    // Convert to 2D coordinates
    uint32_t srcX = src % width_, srcY = src / width_;
    uint32_t dstX = dst % width_, dstY = dst / width_;

    // Calculate distance with wrap-around
    int32_t dx =
        std::abs(static_cast<int32_t>(dstX) - static_cast<int32_t>(srcX));
    int32_t dy =
        std::abs(static_cast<int32_t>(dstY) - static_cast<int32_t>(srcY));

    // Torus wrap-around: min of direct or wrapped distance
    uint32_t wrapDx = std::min(static_cast<uint32_t>(dx), width_ - dx);
    uint32_t wrapDy = std::min(static_cast<uint32_t>(dy), height_ - dy);

    return wrapDx + wrapDy;
  }

  struct RouterNode {
    std::queue<InterconnectMessage> queue;
    mutable std::mutex mutex;
  };

  std::vector<RouterNode> routers_;
  std::atomic<uint64_t> totalMessages_{0};
  std::atomic<uint64_t> totalHops_{0};
};

/**
 * @class InterconnectFactory
 * @brief Factory for creating interconnect instances
 */
class InterconnectFactory {
public:
  static std::unique_ptr<InterconnectInterface>
  create(InterconnectType type, uint32_t numProcessors, uint32_t baseLatency) {

    switch (type) {
    case InterconnectType::Bus:
      return std::make_unique<BusInterconnect>(numProcessors, baseLatency, 64);

    case InterconnectType::Crossbar:
      return std::make_unique<CrossbarInterconnect>(numProcessors, baseLatency);

    case InterconnectType::Mesh: {
      uint32_t width = static_cast<uint32_t>(std::sqrt(numProcessors));
      if (width * width < numProcessors)
        width++;
      return std::make_unique<MeshInterconnect>(numProcessors, baseLatency,
                                                width);
    }

    case InterconnectType::Ring:
      return std::make_unique<RingInterconnect>(numProcessors, baseLatency);

    case InterconnectType::Torus:
      return std::make_unique<TorusInterconnect>(numProcessors, baseLatency);

    default:
      return std::make_unique<BusInterconnect>(numProcessors, baseLatency, 64);
    }
  }
};

} // namespace cachesim
