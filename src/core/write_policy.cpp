/**
 * @file write_policy.cpp
 * @brief Implementation of cache write policies
 * @author Mudit Bhargava
 * @date 2025-05-29
 * @version 1.2.0
 */

#include "write_policy.h"
#include "cache.h"

namespace cachesim {

// WriteThroughPolicy implementation
int WriteThroughPolicy::handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) {
    int writeCount = 1;
    
    if (hit) {
        // Write to current cache
        // Note: actual data update would happen in the cache
        
        // Also write to next level
        if (nextLevel) {
            nextLevel->write(address);
            writeCount++;
        }
    } else {
        // Write miss with write-allocate
        // The cache will handle allocation
        
        // Write to next level
        if (nextLevel) {
            nextLevel->write(address);
            writeCount++;
        }
    }
    
    return writeCount;
}

// WriteThroughNoAllocatePolicy implementation
int WriteThroughNoAllocatePolicy::handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) {
    int writeCount = 0;
    
    if (hit) {
        // Write to current cache
        writeCount = 1;
        
        // Also write to next level
        if (nextLevel) {
            nextLevel->write(address);
            writeCount++;
        }
    } else {
        // Write miss with no-write-allocate
        // Skip current cache, write only to next level
        if (nextLevel) {
            nextLevel->write(address);
            writeCount = 1;
        }
    }
    
    return writeCount;
}

// WriteBackPolicy implementation
int WriteBackPolicy::handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) {
    // Write-back only writes to current cache
    // Data is written to next level only on eviction
    return 1;
}

// WriteBackNoAllocatePolicy implementation
int WriteBackNoAllocatePolicy::handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) {
    if (hit) {
        // Write hit - update current cache
        return 1;
    } else {
        // Write miss with no-write-allocate
        // Skip current cache, write to next level
        if (nextLevel) {
            nextLevel->write(address);
            return 1;
        }
        return 0;
    }
}

} // namespace cachesim
