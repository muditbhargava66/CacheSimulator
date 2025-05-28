#include "mesi_protocol.h"
#include <iostream>
#include <cassert>

using namespace std::string_view_literals;

namespace cachesim {

MESIProtocol::MESIProtocol() {
    // Initialize transition count statistics
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            transitionCount[i][j] = 0;
        }
    }
}

MESIState MESIProtocol::handleLocalRead(MESIState currentState, bool otherCachesHaveCopy) {
    MESIState nextState = currentState;
    
    switch (currentState) {
        case MESIState::Invalid:
            // Cache miss, need to fetch from memory
            // If other caches have a copy, transition to Shared
            // Otherwise, transition to Exclusive
            nextState = otherCachesHaveCopy ? MESIState::Shared : MESIState::Exclusive;
            break;
            
        case MESIState::Modified:
        case MESIState::Exclusive:
        case MESIState::Shared:
            // Already have valid data, stay in current state
            nextState = currentState;
            break;
    }
    
    recordStateTransition(currentState, nextState);
    return nextState;
}

MESIState MESIProtocol::handleLocalWrite(MESIState currentState) {
    MESIState nextState = MESIState::Modified;
    
    switch (currentState) {
        case MESIState::Invalid:
            // Cache miss, need to fetch from memory and invalidate copies in other caches
            nextState = MESIState::Modified;
            break;
            
        case MESIState::Shared:
            // Need to invalidate copies in other caches
            nextState = MESIState::Modified;
            break;
            
        case MESIState::Exclusive:
            // Already exclusive, just mark as modified
            nextState = MESIState::Modified;
            break;
            
        case MESIState::Modified:
            // Already modified, stay in current state
            nextState = MESIState::Modified;
            break;
    }
    
    recordStateTransition(currentState, nextState);
    return nextState;
}

MESIState MESIProtocol::handleRemoteRead(MESIState currentState) {
    MESIState nextState = currentState;
    
    switch (currentState) {
        case MESIState::Modified:
            // Need to provide the modified data and downgrade to Shared
            nextState = MESIState::Shared;
            break;
            
        case MESIState::Exclusive:
            // Need to downgrade to Shared
            nextState = MESIState::Shared;
            break;
            
        case MESIState::Shared:
        case MESIState::Invalid:
            // No change needed
            nextState = currentState;
            break;
    }
    
    recordStateTransition(currentState, nextState);
    return nextState;
}

MESIState MESIProtocol::handleRemoteWrite(MESIState currentState) {
    MESIState nextState = MESIState::Invalid;
    
    // Any remote write invalidates the local copy
    recordStateTransition(currentState, nextState);
    return nextState;
}

MESIState MESIProtocol::handleEviction(MESIState currentState) {
    MESIState nextState = MESIState::Invalid;
    
    // Any eviction results in Invalid state
    recordStateTransition(currentState, nextState);
    return nextState;
}

bool MESIProtocol::requiresWriteback(MESIState state) const {
    // Only Modified state requires writeback
    return state == MESIState::Modified;
}

bool MESIProtocol::isValid(MESIState state) const {
    return state != MESIState::Invalid;
}

bool MESIProtocol::isModified(MESIState state) const {
    return state == MESIState::Modified;
}

std::string_view MESIProtocol::stateToString(MESIState state) const {
    // Using C++17 switch with initializer and std::string_view
    switch (state) {
        case MESIState::Modified:
            return "Modified"sv;
        case MESIState::Exclusive:
            return "Exclusive"sv;
        case MESIState::Shared:
            return "Shared"sv;
        case MESIState::Invalid:
            return "Invalid"sv;
        default:
            return "Unknown"sv;
    }
}

int MESIProtocol::stateToIndex(MESIState state) const {
    switch (state) {
        case MESIState::Modified:
            return 0;
        case MESIState::Exclusive:
            return 1;
        case MESIState::Shared:
            return 2;
        case MESIState::Invalid:
            return 3;
        default:
            assert(false && "Invalid MESI state");
            return -1;
    }
}

void MESIProtocol::recordStateTransition(MESIState from, MESIState to) {
    if (from != to) {
        int fromIdx = stateToIndex(from);
        int toIdx = stateToIndex(to);
        transitionCount[fromIdx][toIdx]++;
    }
}

void MESIProtocol::printStats() const {
    std::cout << "MESI Protocol State Transitions:" << std::endl;
    
    const char* stateNames[4] = {"Modified", "Exclusive", "Shared", "Invalid"};
    
    // Print header
    std::cout << "From\\To  ";
    for (int j = 0; j < 4; ++j) {
        std::cout << stateNames[j] << "\t";
    }
    std::cout << std::endl;
    
    // Print transition counts
    for (int i = 0; i < 4; ++i) {
        std::cout << stateNames[i] << "\t";
        for (int j = 0; j < 4; ++j) {
            std::cout << transitionCount[i][j] << "\t";
        }
        std::cout << std::endl;
    }
}

void MESIProtocol::resetStats() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            transitionCount[i][j] = 0;
        }
    }
}

} // namespace cachesim