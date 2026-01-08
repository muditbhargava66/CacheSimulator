/**
 * @file coherence_protocol.h
 * @brief Abstract base class for cache coherence protocols
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 *
 * Supports MSI, MESI, and MOESI protocols through polymorphic interface.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace cachesim {

/**
 * @enum CoherenceProtocolType
 * @brief Supported coherence protocol types
 */
enum class CoherenceProtocolType { MSI, MESI, MOESI };

/**
 * @class CoherenceProtocolBase
 * @brief Abstract base class for coherence protocol implementations
 */
class CoherenceProtocolBase {
public:
  virtual ~CoherenceProtocolBase() = default;

  /**
   * @brief Handle local read operation
   * @param currentState Current coherence state
   * @param sharedCopy True if other caches have a copy
   * @return New state after operation
   */
  virtual int handleLocalRead(int currentState, bool sharedCopy) = 0;

  /**
   * @brief Handle local write operation
   * @param currentState Current coherence state
   * @return New state after operation
   */
  virtual int handleLocalWrite(int currentState) = 0;

  /**
   * @brief Handle remote read (snoop) from another cache
   * @param currentState Current coherence state
   * @return New state after operation
   */
  virtual int handleRemoteRead(int currentState) = 0;

  /**
   * @brief Handle remote write (snoop) from another cache
   * @param currentState Current coherence state
   * @return New state after operation
   */
  virtual int handleRemoteWrite(int currentState) = 0;

  /**
   * @brief Check if state requires writeback on eviction
   * @param state Current state
   * @return True if writeback needed
   */
  [[nodiscard]] virtual bool requiresWriteback(int state) const = 0;

  /**
   * @brief Check if state is valid (cacheable)
   * @param state Current state
   * @return True if valid
   */
  [[nodiscard]] virtual bool isValid(int state) const = 0;

  /**
   * @brief Get state name for debugging
   * @param state State value
   * @return String representation
   */
  [[nodiscard]] virtual std::string_view stateName(int state) const = 0;

  /**
   * @brief Get protocol name
   * @return Protocol name string
   */
  [[nodiscard]] virtual std::string_view protocolName() const = 0;

  /**
   * @brief Get invalid state value
   * @return Invalid state integer
   */
  [[nodiscard]] virtual int invalidState() const = 0;

  /**
   * @brief Factory method to create protocol instance
   * @param type Protocol type
   * @return Unique pointer to protocol implementation
   */
  static std::unique_ptr<CoherenceProtocolBase>
  create(CoherenceProtocolType type);
};

/**
 * @brief Convert protocol type to string
 */
inline std::string_view protocolTypeName(CoherenceProtocolType type) {
  switch (type) {
  case CoherenceProtocolType::MSI:
    return "MSI";
  case CoherenceProtocolType::MESI:
    return "MESI";
  case CoherenceProtocolType::MOESI:
    return "MOESI";
  default:
    return "Unknown";
  }
}

} // namespace cachesim
