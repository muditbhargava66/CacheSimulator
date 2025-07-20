#pragma once

#include <cstdint>
#include <string>

namespace cachesim {

// MESI Protocol state enumeration
enum class MESIState {
    Modified,  // Modified (M): The cache line is present only in the current cache and has been modified
    Exclusive, // Exclusive (E): The cache line is present only in the current cache and matches memory
    Shared,    // Shared (S): The cache line may be stored in other caches and is clean
    Invalid    // Invalid (I): The cache line is invalid (unused)
};

// MESI Protocol class that handles state transitions
class MESIProtocol {
public:
    MESIProtocol();
    
    // State transitions
    MESIState handleLocalRead(MESIState currentState, bool otherCachesHaveCopy);
    MESIState handleLocalWrite(MESIState currentState);
    MESIState handleRemoteRead(MESIState currentState);
    MESIState handleRemoteWrite(MESIState currentState);
    MESIState handleEviction(MESIState currentState);
    
    // State queries
    [[nodiscard]] bool requiresWriteback(MESIState state) const;
    [[nodiscard]] bool isValid(MESIState state) const;
    [[nodiscard]] bool isModified(MESIState state) const;
    
    // State to string for debugging
    [[nodiscard]] std::string_view stateToString(MESIState state) const;
    
    // Stats tracking
    void recordStateTransition(MESIState from, MESIState to);
    void printStats() const;
    void resetStats();

private:
    // Statistics
    int transitionCount[4][4]; // From state -> To state transitions
    
    // Internal helper methods
    int stateToIndex(MESIState state) const;
};

} // namespace cachesim