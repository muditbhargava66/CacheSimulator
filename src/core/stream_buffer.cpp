#include "stream_buffer.h"
#include <iostream>

namespace cachesim {

StreamBuffer::StreamBuffer(int size)
    : valid(false), 
      buffer(size), 
      lastAccessedIndex(-1),
      hits(0),
      accesses(0) {
}

bool StreamBuffer::access(uint32_t address) {
    accesses++;
    
    auto it = std::find(buffer.begin(), buffer.end(), address);
    if (it != buffer.end()) {
        lastAccessedIndex = std::distance(buffer.begin(), it);
        hits++;
        return true;
    }
    
    return false;
}

void StreamBuffer::prefetch(uint32_t address) {
    valid = true;
    lastAccessedIndex = 0;
    buffer[0] = address;
    
    // Prefetch sequential blocks
    for (size_t i = 1; i < buffer.size(); ++i) {
        buffer[i] = address + i;
    }
}

void StreamBuffer::shift() {
    if (lastAccessedIndex >= 0) {
        // Shift out accessed blocks
        buffer.erase(buffer.begin(), buffer.begin() + lastAccessedIndex + 1);
        buffer.resize(buffer.size(), 0);
        lastAccessedIndex = -1;
    }
}

double StreamBuffer::getHitRate() const {
    if (accesses == 0) {
        return 0.0;
    }
    
    return static_cast<double>(hits) / accesses;
}

void StreamBuffer::resetStats() {
    hits = 0;
    accesses = 0;
}

void StreamBuffer::printStats() const {
    std::cout << "Stream Buffer Statistics:" << std::endl;
    std::cout << "  Buffer Size: " << buffer.size() << std::endl;
    std::cout << "  Valid: " << (valid ? "Yes" : "No") << std::endl;
    std::cout << "  Hits: " << hits << std::endl;
    std::cout << "  Accesses: " << accesses << std::endl;
    std::cout << "  Hit Rate: " << (getHitRate() * 100.0) << "%" << std::endl;
}

} // namespace cachesim