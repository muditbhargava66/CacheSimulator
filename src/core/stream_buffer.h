#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>

namespace cachesim {

// StreamBuffer class for implementing prefetching
class StreamBuffer {
public:
    // Constructor
    StreamBuffer(int size);
    
    // Check if address is in the stream buffer
    bool access(uint32_t address);
    
    // Prefetch a sequence of addresses starting from the given address
    void prefetch(uint32_t address);
    
    // Shift out accessed blocks from the buffer
    void shift();
    
    // Getters
    [[nodiscard]] bool isValid() const { return valid; }
    [[nodiscard]] int getLastAccessedIndex() const { return lastAccessedIndex; }
    [[nodiscard]] const std::vector<uint32_t>& getBuffer() const { return buffer; }
    
    // Statistics
    int getHits() const { return hits; }
    int getAccesses() const { return accesses; }
    double getHitRate() const;
    void resetStats();
    void printStats() const;

private:
    bool valid;                // Is the buffer valid?
    std::vector<uint32_t> buffer; // The prefetched addresses
    int lastAccessedIndex;     // Index of the last accessed address
    
    // Statistics
    int hits;                  // Number of hits
    int accesses;              // Number of accesses
};

} // namespace cachesim