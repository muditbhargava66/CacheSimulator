/**
 * @file msi_protocol.cpp
 * @brief MSI cache coherence protocol implementation
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include "msi_protocol.h"
#include <iostream>

namespace cachesim {

MSIProtocol::MSIProtocol() { resetStats(); }

int MSIProtocol::handleLocalRead(int currentState,
                                 [[maybe_unused]] bool sharedCopy) {
  MSIState state = static_cast<MSIState>(currentState);
  MSIState newState = state;

  switch (state) {
  case MSIState::Modified:
  case MSIState::Shared:
    // Already have valid copy
    newState = state;
    break;

  case MSIState::Invalid:
    // Fetch from memory - go to Shared (MSI has no Exclusive)
    newState = MSIState::Shared;
    break;
  }

  recordStateTransition(state, newState);
  return static_cast<int>(newState);
}

int MSIProtocol::handleLocalWrite(int currentState) {
  MSIState state = static_cast<MSIState>(currentState);
  MSIState newState = state;

  switch (state) {
  case MSIState::Modified:
    // Already Modified
    newState = MSIState::Modified;
    break;

  case MSIState::Shared:
  case MSIState::Invalid:
    // Upgrade/fetch to Modified
    newState = MSIState::Modified;
    break;
  }

  recordStateTransition(state, newState);
  return static_cast<int>(newState);
}

int MSIProtocol::handleRemoteRead(int currentState) {
  MSIState state = static_cast<MSIState>(currentState);
  MSIState newState = state;

  switch (state) {
  case MSIState::Modified:
    // Downgrade to Shared (must writeback)
    newState = MSIState::Shared;
    break;

  case MSIState::Shared:
  case MSIState::Invalid:
    // No change
    newState = state;
    break;
  }

  recordStateTransition(state, newState);
  return static_cast<int>(newState);
}

int MSIProtocol::handleRemoteWrite(int currentState) {
  MSIState state = static_cast<MSIState>(currentState);
  MSIState newState = state;

  switch (state) {
  case MSIState::Modified:
  case MSIState::Shared:
    // Must invalidate
    newState = MSIState::Invalid;
    break;

  case MSIState::Invalid:
    // Already invalid
    newState = MSIState::Invalid;
    break;
  }

  recordStateTransition(state, newState);
  return static_cast<int>(newState);
}

bool MSIProtocol::requiresWriteback(int state) const {
  return state == static_cast<int>(MSIState::Modified);
}

bool MSIProtocol::isValid(int state) const {
  return state != static_cast<int>(MSIState::Invalid);
}

std::string_view MSIProtocol::stateName(int state) const {
  if (state >= 0 && state < NUM_STATES) {
    return stateNames_[state];
  }
  return "Unknown";
}

void MSIProtocol::recordStateTransition(MSIState from, MSIState to) {
  int fromIdx = static_cast<int>(from);
  int toIdx = static_cast<int>(to);
  if (fromIdx >= 0 && fromIdx < NUM_STATES && toIdx >= 0 &&
      toIdx < NUM_STATES) {
    transitionCount_[fromIdx][toIdx]++;
  }
}

void MSIProtocol::printStats() const {
  std::cout << "\nMSI Protocol Statistics:\n";
  std::cout << "State Transitions:\n";
  std::cout << "From\\To    ";
  for (int i = 0; i < NUM_STATES; ++i) {
    std::cout << stateNames_[i].substr(0, 3) << "   ";
  }
  std::cout << "\n";

  for (int i = 0; i < NUM_STATES; ++i) {
    std::cout << stateNames_[i].substr(0, 8);
    for (size_t j = stateNames_[i].size(); j < 10; ++j)
      std::cout << " ";
    for (int j = 0; j < NUM_STATES; ++j) {
      std::cout << transitionCount_[i][j] << "     ";
    }
    std::cout << "\n";
  }
}

void MSIProtocol::resetStats() {
  for (auto &row : transitionCount_) {
    row.fill(0);
  }
}

} // namespace cachesim
