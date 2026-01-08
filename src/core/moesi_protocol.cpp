/**
 * @file moesi_protocol.cpp
 * @brief MOESI cache coherence protocol implementation
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include "moesi_protocol.h"
#include <iostream>

namespace cachesim {

MOESIProtocol::MOESIProtocol() { resetStats(); }

int MOESIProtocol::handleLocalRead(int currentState, bool sharedCopy) {
  MOESIState state = static_cast<MOESIState>(currentState);
  MOESIState newState = state;

  switch (state) {
  case MOESIState::Modified:
  case MOESIState::Owned:
  case MOESIState::Exclusive:
  case MOESIState::Shared:
    // Already have valid copy, state unchanged
    newState = state;
    break;

  case MOESIState::Invalid:
    // Need to fetch from memory/other cache
    if (sharedCopy) {
      // Other cache has copy - go to Shared
      newState = MOESIState::Shared;
    } else {
      // No other copy - go to Exclusive
      newState = MOESIState::Exclusive;
    }
    break;
  }

  recordStateTransition(state, newState);
  return static_cast<int>(newState);
}

int MOESIProtocol::handleLocalWrite(int currentState) {
  MOESIState state = static_cast<MOESIState>(currentState);
  MOESIState newState = state;

  switch (state) {
  case MOESIState::Modified:
    // Already Modified, stay Modified
    newState = MOESIState::Modified;
    break;

  case MOESIState::Owned:
  case MOESIState::Exclusive:
  case MOESIState::Shared:
    // Upgrade to Modified (invalidate other copies)
    newState = MOESIState::Modified;
    break;

  case MOESIState::Invalid:
    // Fetch and modify - go to Modified
    newState = MOESIState::Modified;
    break;
  }

  recordStateTransition(state, newState);
  return static_cast<int>(newState);
}

int MOESIProtocol::handleRemoteRead(int currentState) {
  MOESIState state = static_cast<MOESIState>(currentState);
  MOESIState newState = state;

  switch (state) {
  case MOESIState::Modified:
    // Downgrade to Owned - keep dirty responsibility
    newState = MOESIState::Owned;
    break;

  case MOESIState::Owned:
    // Stay Owned - still responsible for data
    newState = MOESIState::Owned;
    break;

  case MOESIState::Exclusive:
    // Downgrade to Shared
    newState = MOESIState::Shared;
    break;

  case MOESIState::Shared:
  case MOESIState::Invalid:
    // No change needed
    newState = state;
    break;
  }

  recordStateTransition(state, newState);
  return static_cast<int>(newState);
}

int MOESIProtocol::handleRemoteWrite(int currentState) {
  MOESIState state = static_cast<MOESIState>(currentState);
  MOESIState newState = state;

  switch (state) {
  case MOESIState::Modified:
  case MOESIState::Owned:
    // Must writeback and invalidate
    newState = MOESIState::Invalid;
    break;

  case MOESIState::Exclusive:
  case MOESIState::Shared:
    // Just invalidate
    newState = MOESIState::Invalid;
    break;

  case MOESIState::Invalid:
    // Already invalid
    newState = MOESIState::Invalid;
    break;
  }

  recordStateTransition(state, newState);
  return static_cast<int>(newState);
}

bool MOESIProtocol::requiresWriteback(int state) const {
  MOESIState s = static_cast<MOESIState>(state);
  return s == MOESIState::Modified || s == MOESIState::Owned;
}

bool MOESIProtocol::isValid(int state) const {
  MOESIState s = static_cast<MOESIState>(state);
  return s != MOESIState::Invalid;
}

std::string_view MOESIProtocol::stateName(int state) const {
  if (state >= 0 && state < NUM_STATES) {
    return stateNames_[state];
  }
  return "Unknown";
}

void MOESIProtocol::recordStateTransition(MOESIState from, MOESIState to) {
  int fromIdx = static_cast<int>(from);
  int toIdx = static_cast<int>(to);
  if (fromIdx >= 0 && fromIdx < NUM_STATES && toIdx >= 0 &&
      toIdx < NUM_STATES) {
    transitionCount_[fromIdx][toIdx]++;
  }
}

void MOESIProtocol::printStats() const {
  std::cout << "\nMOESI Protocol Statistics:\n";
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

void MOESIProtocol::resetStats() {
  for (auto &row : transitionCount_) {
    row.fill(0);
  }
}

} // namespace cachesim
