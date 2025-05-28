#include "cache.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>
#include <numeric>
#include <sstream>

namespace cachesim {

    Cache::Cache(const CacheConfig& config)
    : size(config.size),
      associativity(config.associativity),
      blockSize(config.blockSize),
      replacementPolicy(config.replacementPolicy),
      writePolicy(config.writePolicy),
      streamBuffer(config.prefetchDistance),
      prefetchEnabled(config.prefetchEnabled),
      hits(0),
      misses(0),
      reads(0),
      writes(0),
      writeBacks(0),
      writeThroughs(0),
      missTypeStats{0, 0, 0, 0} {
    
    // Configuration validation is done in CacheConfig constructor
    // Calculate number of sets
    numSets = size / (associativity * blockSize);
    
    // Additional validation
    if (numSets <= 0) {
        throw CacheConfigError("Invalid configuration: number of sets must be positive");
    }
    
    // Initialize cache sets
    sets.resize(numSets);
    for (auto& set : sets) {
        set.blocks.resize(associativity);
        set.lruOrder.resize(associativity);
        set.fifoOrder.resize(associativity);
        
        // Initialize LRU and FIFO order
        std::iota(set.lruOrder.begin(), set.lruOrder.end(), 0);
        std::iota(set.fifoOrder.begin(), set.fifoOrder.end(), 0);
        set.nextFifoIndex = 0;
    }
}

bool Cache::access(uint32_t address, bool isWrite, 
                  std::optional<Cache*> nextLevel,
                  std::optional<StridePredictor*> stridePredictor) {
    
    // Count access type
    if (isWrite) {
        writes++;
    } else {
        reads++;
    }
    
    // Get tag and set index for this address
    auto [tag, setIndex] = getTagAndSet(address);
    auto& set = sets[setIndex];
    
    // Check if the block is in the cache
    auto result = findBlock(address);
    
    if (auto blockIndex = std::get_if<int>(&result)) {
        // Cache hit
        hits++;
        
        // Handle the hit based on the access type
        auto& block = set.blocks[*blockIndex];
        
        if (isWrite) {
            if (writePolicy == WritePolicy::WriteThrough) {
                // Write-through: write to both cache and next level
                writeThrough(address, nextLevel.value_or(nullptr));
                // Block is not marked dirty in write-through
                block.dirty = false;
            } else {
                // Write-back: mark block as dirty and modified
                block.dirty = true;
                block.mesiState = MESIState::Modified;
            }
        }
        
        // Update replacement policy state
        updateReplacementPolicy(set, *blockIndex);
        
        return true;
    } else {
        // Cache miss
        misses++;
        
        // Increment miss type stat
        auto missType = std::get<CacheMissType>(result);
        missTypeStats[static_cast<int>(missType)]++;
        
        // Check stream buffer for prefetched blocks
        if (prefetchEnabled && streamBuffer.access(address)) {
            // Stream buffer hit - prefetch was successful
            // TODO: Consider using logger instead of direct cout
            if constexpr (false) { // Disable verbose output in production
                std::cout << "Stream buffer hit: 0x" << std::hex << address << std::dec << std::endl;
            }
            
            if (!isWrite) {
                // Shift consumed blocks from stream buffer
                streamBuffer.shift();
            }
            
            return true;
        }
        
        // Select victim block to replace
        int victimIndex = findVictim(set);
        auto& victimBlock = set.blocks[victimIndex];
        
        // Handle writeback if the victim is dirty
        if (victimBlock.valid && victimBlock.requiresWriteback()) {
            uint32_t victimAddress = (victimBlock.tag * numSets + setIndex) * blockSize;
            writeBack(victimAddress, nextLevel.value_or(nullptr));
        }
        
        // Install the new block
        installBlock(address, setIndex, victimIndex, isWrite, nextLevel.value_or(nullptr));
        
        // Update replacement policy order
        updateReplacementPolicy(set, victimIndex);
        
        // Handle prefetching if enabled
        if (prefetchEnabled && stridePredictor) {
            handlePrefetch(address, isWrite, nextLevel.value_or(nullptr), stridePredictor.value_or(nullptr));
        }
        
        return false;
    }
}

// Helper to get tag and set index for an address
std::pair<uint32_t, int> Cache::getTagAndSet(uint32_t address) const {
    uint32_t tag = address / blockSize;
    int setIndex = (address / blockSize) % numSets;
    return {tag, setIndex};
}

// Update LRU ordering in a set
void Cache::updateLRU(CacheSet& set, int accessedIndex) {
    // Move the accessed block to the front of the LRU order (most recently used)
    auto it = std::find(set.lruOrder.begin(), set.lruOrder.end(), accessedIndex);
    assert(it != set.lruOrder.end() && "Block index not found in LRU order");
    
    // Use C++17 splicing to efficiently move the element
    set.lruOrder.erase(it);
    set.lruOrder.insert(set.lruOrder.begin(), accessedIndex);
}

// Find a victim block for replacement
int Cache::findVictim(const CacheSet& set) const {
    switch (replacementPolicy) {
        case ReplacementPolicy::LRU:
            return findVictimLRU(set);
        case ReplacementPolicy::FIFO:
            return findVictimFIFO(set);
        case ReplacementPolicy::Random:
            return findVictimRandom(set);
        default:
            return findVictimLRU(set); // Default to LRU
    }
}

// Find victim using LRU policy
int Cache::findVictimLRU(const CacheSet& set) const {
    return set.lruOrder.back();
}

// Find victim using FIFO policy
int Cache::findVictimFIFO(const CacheSet& set) const {
    // Find the first valid block in FIFO order
    for (int idx : set.fifoOrder) {
        if (set.blocks[idx].valid) {
            return idx;
        }
    }
    // If no valid blocks, return the first slot
    return 0;
}

// Find victim using Random policy
int Cache::findVictimRandom(const CacheSet& set) const {
    // Find all valid blocks
    std::vector<int> validIndices;
    for (int i = 0; i < associativity; ++i) {
        if (set.blocks[i].valid) {
            validIndices.push_back(i);
        }
    }
    
    if (validIndices.empty()) {
        // No valid blocks, return random index
        std::uniform_int_distribution<int> dist(0, associativity - 1);
        return dist(rng);
    } else {
        // Return random valid block
        std::uniform_int_distribution<int> dist(0, validIndices.size() - 1);
        return validIndices[dist(rng)];
    }
}

// Install a new block in the cache
void Cache::installBlock(uint32_t address, int setIndex, int blockIndex, bool isWrite, Cache* nextLevel) {
    auto [tag, _] = getTagAndSet(address);
    auto& block = sets[setIndex].blocks[blockIndex];
    
    // Retrieve the block from the next level if available
    if (nextLevel) {
        [[maybe_unused]] bool hitStatus = nextLevel->access(address, false, std::nullopt);
    }
    
    // Initialize the block
    block.valid = true;
    block.dirty = isWrite;
    block.tag = tag;
    
    // Set MESI state based on access type
    if (isWrite) {
        block.mesiState = MESIState::Modified;
    } else {
        // If no other caches have this block, it's Exclusive
        // Otherwise, it's Shared (in a multi-cache system)
        // For simplicity, we assume it's Exclusive here
        block.mesiState = MESIState::Exclusive;
    }
}

// Handle writeback to next level
void Cache::writeBack(uint32_t address, Cache* nextLevel) {
    writeBacks++;
    
    if (nextLevel) {
        static_cast<void>(nextLevel->access(address, true, std::nullopt));
    }
    // In a real system, would eventually write to main memory
}

// Find a block in the cache and classify hits/misses
Cache::AccessResult Cache::findBlock(uint32_t address) const {
    auto [tag, setIndex] = getTagAndSet(address);
    const auto& set = sets[setIndex];
    
    // Check if the block is in the cache
    for (int i = 0; i < associativity; ++i) {
        const auto& block = set.blocks[i];
        if (block.valid && block.tag == tag) {
            // Block found
            return i; // Return block index for a hit
        }
    }
    
    // Block not found - classify the miss type
    bool setHasInvalid = false;
    for (const auto& block : set.blocks) {
        if (!block.valid) {
            setHasInvalid = true;
            break;
        }
    }
    
    if (setHasInvalid) {
        // Compulsory miss - there's space in the set
        return CacheMissType::Compulsory;
    } else if (associativity < numSets) {
        // Conflict miss - specific set is full, but entire cache may not be
        return CacheMissType::Conflict;
    } else {
        // Capacity miss - entire cache is full
        return CacheMissType::Capacity;
    }
}

// Handle prefetching logic
void Cache::handlePrefetch(uint32_t address, bool isWrite, Cache* nextLevel, StridePredictor* stridePredictor) {
    if (!isWrite && stridePredictor) {
        // Get the predicted stride
        int32_t stride = stridePredictor->getStride(address);
        
        if (stride != 0) {
            // Calculate prefetch address using stride prediction
            uint32_t prefetchAddress = address + stride;
            
            // Prefetch action - stride-based prefetching triggered
            // TODO: Consider using logger for prefetch diagnostics
            if constexpr (false) { // Disable verbose output in production
                std::cout << "Prefetching address (stride-based): 0x" 
                          << std::hex << prefetchAddress << std::dec << std::endl;
            }
            
            // Access the prefetch address in the next level to bring it closer
            if (nextLevel) {
                static_cast<void>(nextLevel->access(prefetchAddress, false, std::nullopt, stridePredictor));
            }
            
            // Update stream buffer with the prefetched address
            streamBuffer.prefetch(prefetchAddress);
        }
    }
}

// Invalidate a block (for coherence)
void Cache::invalidateBlock(uint32_t address) {
    auto [tag, setIndex] = getTagAndSet(address);
    auto& set = sets[setIndex];
    
    for (auto& block : set.blocks) {
        if (block.valid && block.tag == tag) {
            // Update MESI state to Invalid
            block.mesiState = mesiProtocol.handleRemoteWrite(block.mesiState);
            if (block.mesiState == MESIState::Invalid) {
                block.valid = false;
                // Count as a coherence miss for future accesses
                missTypeStats[static_cast<int>(CacheMissType::Coherence)]++;
            }
            break;
        }
    }
}

// Update the MESI state of a block
void Cache::updateMESIState(uint32_t address, MESIState newState) {
    auto [tag, setIndex] = getTagAndSet(address);
    auto& set = sets[setIndex];
    
    for (auto& block : set.blocks) {
        if (block.valid && block.tag == tag) {
            MESIState oldState = block.mesiState;
            block.mesiState = newState;
            mesiProtocol.recordStateTransition(oldState, newState);
            
            // If block becomes invalid, update valid flag
            if (newState == MESIState::Invalid) {
                block.valid = false;
            }
            
            break;
        }
    }
}

// Print cache statistics
void Cache::printStats() const {
    std::cout << "Cache Statistics:" << std::endl;
    std::cout << "----------------" << std::endl;
    std::cout << "  Size: " << size << " bytes" << std::endl;
    std::cout << "  Associativity: " << associativity << "-way" << std::endl;
    std::cout << "  Block Size: " << blockSize << " bytes" << std::endl;
    std::cout << "  Number of Sets: " << numSets << std::endl;
    std::cout << "  Prefetching: " << (prefetchEnabled ? "Enabled" : "Disabled") << std::endl;
    
    if (prefetchEnabled) {
        std::cout << "  Stream Buffer Statistics:" << std::endl;
        streamBuffer.printStats();
    }
    
    std::cout << std::endl;
    std::cout << "Access Statistics:" << std::endl;
    std::cout << "  Hits: " << hits << std::endl;
    std::cout << "  Misses: " << misses << std::endl;
    std::cout << "  Reads: " << reads << std::endl;
    std::cout << "  Writes: " << writes << std::endl;
    std::cout << "  Write-backs: " << writeBacks << std::endl;
    std::cout << "  Hit Ratio: " << std::fixed << std::setprecision(2) 
              << (100.0 * hits / (hits + misses)) << "%" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Miss Type Statistics:" << std::endl;
    for (int i = 0; i < 4; ++i) {
        std::cout << "  " << CacheMissTypeNames[i] << " Misses: " 
                  << missTypeStats[i] << " (" 
                  << std::fixed << std::setprecision(2)
                  << (misses > 0 ? 100.0 * missTypeStats[i] / misses : 0.0) 
                  << "%)" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "MESI Protocol Statistics:" << std::endl;
    mesiProtocol.printStats();
}

// Reset all statistics
void Cache::resetStats() {
    hits = 0;
    misses = 0;
    reads = 0;
    writes = 0;
    writeBacks = 0;
    std::fill(missTypeStats.begin(), missTypeStats.end(), 0);
    
    // Reset stream buffer stats
    streamBuffer.resetStats();
}

// Export cache state for debugging (v1.1.0)
std::string Cache::exportCacheState() const {
    std::ostringstream oss;
    oss << "Cache State Export\n";
    oss << "==================\n";
    oss << "Configuration:\n";
    oss << "  Size: " << size << " bytes\n";
    oss << "  Associativity: " << associativity << "-way\n";
    oss << "  Block Size: " << blockSize << " bytes\n";
    oss << "  Number of Sets: " << numSets << "\n\n";
    
    oss << "Cache Contents:\n";
    for (int setIdx = 0; setIdx < numSets; ++setIdx) {
        const auto& set = sets[setIdx];
        bool hasValidBlocks = false;
        
        for (int way = 0; way < associativity; ++way) {
            const auto& block = set.blocks[way];
            if (block.valid) {
                hasValidBlocks = true;
                break;
            }
        }
        
        if (hasValidBlocks) {
            oss << "Set " << setIdx << ":\n";
            for (int way = 0; way < associativity; ++way) {
                const auto& block = set.blocks[way];
                if (block.valid) {
                    oss << "  Way " << way << ": ";
                    oss << "Tag=0x" << std::hex << block.tag << std::dec;
                    oss << ", State=" << (block.dirty ? "Dirty" : "Clean");
                    oss << ", MESI=" << static_cast<int>(block.mesiState);
                    oss << "\n";
                }
            }
        }
    }
    
    oss << "\nStatistics:\n";
    oss << "  Hits: " << hits << "\n";
    oss << "  Misses: " << misses << "\n";
    oss << "  Hit Ratio: " << std::fixed << std::setprecision(2) 
        << (getHitRatio() * 100.0) << "%\n";
    oss << "  Efficiency: " << std::fixed << std::setprecision(2)
        << (getCacheEfficiency() * 100.0) << "%\n";
    
    return oss.str();
}

// Cache warmup for benchmarking (v1.1.0)
void Cache::warmup(const std::vector<uint32_t>& addresses) {
    // Save current statistics
    int savedHits = hits;
    int savedMisses = misses;
    int savedReads = reads;
    int savedWrites = writes;
    int savedWriteBacks = writeBacks;
    auto savedMissTypeStats = missTypeStats;
    
    // Perform warmup accesses
    for (uint32_t addr : addresses) {
        static_cast<void>(access(addr, false, std::nullopt, std::nullopt));
    }
    
    // Restore statistics (warmup doesn't count)
    hits = savedHits;
    misses = savedMisses;
    reads = savedReads;
    writes = savedWrites;
    writeBacks = savedWriteBacks;
    missTypeStats = savedMissTypeStats;
}

} // namespace cachesim