/**
 * @file moesi_protocol.h
 * @brief MOESI cache coherence protocol implementation
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 *
 * MOESI adds the Owned state to MESI, allowing dirty sharing without
 * writeback to memory. This is more efficient for shared dirty data.
 *
 * States:
 * - Modified (M): Exclusive, dirty - must writeback
 * - Owned (O): Shared, dirty - responsible for writeback
 * - Exclusive (E): Exclusive, clean - no writeback needed
 * - Shared (S): Shared, clean - no writeback needed
 * - Invalid (I): Not present
 */

#pragma once

#include "coherence_protocol.h"
#include <array>

namespace cachesim {

/**
 * @enum MOESIState
 * @brief MOESI protocol states
 */
enum class MOESIState : int {
  Modified = 0,  // Exclusive dirty
  Owned = 1,     // Shared dirty (responsible for writeback)
  Exclusive = 2, // Exclusive clean
  Shared = 3,    // Shared clean
  Invalid = 4    // Not present
};

/**
 * @class MOESIProtocol
 * @brief MOESI coherence protocol implementation
 */
class MOESIProtocol : public CoherenceProtocolBase {
public:
  MOESIProtocol();

  int handleLocalRead(int currentState, bool sharedCopy) override;
  int handleLocalWrite(int currentState) override;
  int handleRemoteRead(int currentState) override;
  int handleRemoteWrite(int currentState) override;

  [[nodiscard]] bool requiresWriteback(int state) const override;
  [[nodiscard]] bool isValid(int state) const override;
  [[nodiscard]] std::string_view stateName(int state) const override;
  [[nodiscard]] std::string_view protocolName() const override {
    return "MOESI";
  }
  [[nodiscard]] int invalidState() const override {
    return static_cast<int>(MOESIState::Invalid);
  }

  // MOESI-specific methods
  [[nodiscard]] bool isOwned(int state) const {
    return state == static_cast<int>(MOESIState::Owned);
  }

  [[nodiscard]] bool canSupplyData(int state) const {
    return state == static_cast<int>(MOESIState::Modified) ||
           state == static_cast<int>(MOESIState::Owned) ||
           state == static_cast<int>(MOESIState::Exclusive);
  }

  // Statistics
  void recordStateTransition(MOESIState from, MOESIState to);
  void printStats() const;
  void resetStats();

private:
  static constexpr int NUM_STATES = 5;
  std::array<std::array<int, NUM_STATES>, NUM_STATES> transitionCount_{};

  static constexpr std::array<std::string_view, NUM_STATES> stateNames_ = {
      "Modified", "Owned", "Exclusive", "Shared", "Invalid"};
};

} // namespace cachesim
