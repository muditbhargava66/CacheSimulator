#pragma once

#include <vector>
#include <cstdint>
#include "mesi_protocol.h"

namespace cachesim {

// Enhanced cache block structure with MESI protocol support
struct CacheBlock {
    bool valid;           // Is the block valid?
    bool dirty;           // Has the block been modified?
    uint32_t tag;         // Tag bits for address lookup
    std::vector<uint8_t> data; // Data stored in this block
    MESIState mesiState;  // Current MESI protocol state
    
    // Visualization support fields (v1.2.0)
    uint32_t accessCount;    // Number of times this block was accessed
    uint64_t lastAccess;     // Timestamp of last access
    bool prefetched;         // Whether this block was prefetched
    
    // Default constructor
    CacheBlock() 
        : valid(false), 
          dirty(false), 
          tag(0), 
          mesiState(MESIState::Invalid),
          accessCount(0),
          lastAccess(0),
          prefetched(false) {}
    
    // Constructor with parameters
    CacheBlock(bool valid, bool dirty, uint32_t tag) 
        : valid(valid), 
          dirty(dirty), 
          tag(tag), 
          mesiState(valid ? MESIState::Exclusive : MESIState::Invalid),
          accessCount(0),
          lastAccess(0),
          prefetched(false) {}
    
    // Convenience method to check if block is in modified state
    bool isModified() const {
        return mesiState == MESIState::Modified;
    }
    
    // Convenience method to check if block requires writeback on eviction
    bool requiresWriteback() const {
        return dirty || mesiState == MESIState::Modified;
    }
};

// Cache set structure with blocks and replacement policy information
struct CacheSet {
    std::vector<CacheBlock> blocks;
    std::vector<int> lruOrder;      // For LRU policy
    std::vector<int> fifoOrder;     // For FIFO policy (v1.1.0)
    int nextFifoIndex = 0;          // Next index for FIFO replacement (v1.1.0)
};

} // namespace cachesim