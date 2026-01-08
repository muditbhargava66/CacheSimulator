/**
 * @file msi_protocol.h
 * @brief MSI cache coherence protocol implementation
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 *
 * MSI is the simplest snooping coherence protocol.
 *
 * States:
 * - Modified (M): Exclusive, dirty - must writeback
 * - Shared (S): Shared, clean
 * - Invalid (I): Not present
 */

#pragma once

#include "coherence_protocol.h"
#include <array>

namespace cachesim {

/**
 * @enum MSIState
 * @brief MSI protocol states
 */
enum class MSIState : int {
  Modified = 0, // Exclusive dirty
  Shared = 1,   // Shared clean
  Invalid = 2   // Not present
};

/**
 * @class MSIProtocol
 * @brief MSI coherence protocol implementation
 */
class MSIProtocol : public CoherenceProtocolBase {
public:
  MSIProtocol();

  int handleLocalRead(int currentState, bool sharedCopy) override;
  int handleLocalWrite(int currentState) override;
  int handleRemoteRead(int currentState) override;
  int handleRemoteWrite(int currentState) override;

  [[nodiscard]] bool requiresWriteback(int state) const override;
  [[nodiscard]] bool isValid(int state) const override;
  [[nodiscard]] std::string_view stateName(int state) const override;
  [[nodiscard]] std::string_view protocolName() const override { return "MSI"; }
  [[nodiscard]] int invalidState() const override {
    return static_cast<int>(MSIState::Invalid);
  }

  void recordStateTransition(MSIState from, MSIState to);
  void printStats() const;
  void resetStats();

private:
  static constexpr int NUM_STATES = 3;
  std::array<std::array<int, NUM_STATES>, NUM_STATES> transitionCount_{};

  static constexpr std::array<std::string_view, NUM_STATES> stateNames_ = {
      "Modified", "Shared", "Invalid"};
};

} // namespace cachesim
