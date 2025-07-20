#include "cache.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <string>
#include <numeric>
#include <sstream>
#include <atomic>

namespace cachesim {

// Global access counter for timestamps
static std::atomic<uint64_t> globalAccessCounter{0};

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
    replacementPolicies.resize(numSets);
    
    for (int setIdx = 0; setIdx < numSets; ++setIdx) {
        auto& set = sets[setIdx];
        set.blocks.resize(associativity);
        set.lruOrder.resize(associativity);
        set.fifoOrder.resize(associativity);
        
        // Initialize LRU and FIFO order
        std::iota(set.lruOrder.begin(), set.lruOrder.end(), 0);
        std::iota(set.fifoOrder.begin(), set.fifoOrder.end(), 0);
        set.nextFifoIndex = 0;
        
        // Create replacement policy for this set
        replacementPolicies[setIdx] = ReplacementPolicyFactory::create(replacementPolicy, associativity);
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
        
        // Update block access statistics (v1.2.0)
        block.accessCount++;
        block.lastAccess = globalAccessCounter.fetch_add(1);
        
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
        replacementPolicies[setIndex]->onAccess(*blockIndex);
        
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
        int victimIndex = findVictim(set, setIndex);
        auto& victimBlock = set.blocks[victimIndex];
        
        // Handle writeback if the victim is dirty
        if (victimBlock.valid && victimBlock.requiresWriteback()) {
            uint32_t victimAddress = (victimBlock.tag * numSets + setIndex) * blockSize;
            writeBack(victimAddress, nextLevel.value_or(nullptr));
        }
        
        // Install the new block
        installBlock(address, setIndex, victimIndex, isWrite, nextLevel.value_or(nullptr), false);
        
        // Update replacement policy for installed block
        replacementPolicies[setIndex]->onInstall(victimIndex);
        
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

// Find a victim block for replacement
int Cache::findVictim(const CacheSet& set, int setIndex) const {
    // Create valid blocks vector
    std::vector<bool> validBlocks(associativity);
    for (int i = 0; i < associativity; ++i) {
        validBlocks[i] = set.blocks[i].valid;
    }
    
    // Use replacement policy to select victim
    return replacementPolicies[setIndex]->selectVictim(validBlocks);
}

// Install a new block in the cache
void Cache::installBlock(uint32_t address, int setIndex, int blockIndex, bool isWrite, Cache* nextLevel, bool isPrefetch) {
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
    
    // Initialize access statistics (v1.2.0)
    block.accessCount = 1;
    block.lastAccess = globalAccessCounter.fetch_add(1);
    block.prefetched = isPrefetch;
    
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

// Handle write-through (v1.1.0)
void Cache::writeThrough(uint32_t address, Cache* nextLevel) {
    writeThroughs++;
    
    if (nextLevel) {
        static_cast<void>(nextLevel->access(address, true, std::nullopt));
    }
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
            
            // Check if the prefetch address is already in the cache
            auto [tag, setIndex] = getTagAndSet(prefetchAddress);
            auto result = findBlock(prefetchAddress);
            
            if (!std::holds_alternative<int>(result)) {
                // Block not in cache, install it as a prefetch
                auto& set = sets[setIndex];
                int victimIndex = findVictim(set, setIndex);
                auto& victimBlock = set.blocks[victimIndex];
                
                // Handle writeback if the victim is dirty
                if (victimBlock.valid && victimBlock.requiresWriteback()) {
                    uint32_t victimAddress = (victimBlock.tag * numSets + setIndex) * blockSize;
                    writeBack(victimAddress, nextLevel);
                }
                
                // Install the prefetched block
                installBlock(prefetchAddress, setIndex, victimIndex, false, nextLevel, true);
                
                // Update replacement policy
                replacementPolicies[setIndex]->onInstall(victimIndex);
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
    std::cout << "  Replacement Policy: " << replacementPolicyToString(replacementPolicy) << std::endl;
    std::cout << "  Write Policy: " << (writePolicy == WritePolicy::WriteBack ? "Write-back" : "Write-through") << std::endl;
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
    if (writePolicy == WritePolicy::WriteThrough) {
        std::cout << "  Write-throughs: " << writeThroughs << std::endl;
    }
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
    writeThroughs = 0;
    std::fill(missTypeStats.begin(), missTypeStats.end(), 0);
    
    // Reset stream buffer stats
    streamBuffer.resetStats();
    
    // Reset MESI protocol stats
    mesiProtocol.resetStats();
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
    oss << "  Number of Sets: " << numSets << "\n";
    oss << "  Replacement Policy: " << replacementPolicyToString(replacementPolicy) << "\n";
    oss << "  Write Policy: " << (writePolicy == WritePolicy::WriteBack ? "Write-back" : "Write-through") << "\n\n";
    
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
                    oss << ", MESI=" << mesiProtocol.stateToString(block.mesiState);
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
    int savedWriteThroughs = writeThroughs;
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
    writeThroughs = savedWriteThroughs;
    missTypeStats = savedMissTypeStats;
}

// Block state access methods for visualization (v1.2.0)
bool Cache::isBlockValid(uint32_t setIndex, uint32_t wayIndex) const {
    if (setIndex >= static_cast<uint32_t>(numSets) || wayIndex >= static_cast<uint32_t>(associativity)) {
        return false;
    }
    return sets[setIndex].blocks[wayIndex].valid;
}

bool Cache::isBlockDirty(uint32_t setIndex, uint32_t wayIndex) const {
    if (setIndex >= static_cast<uint32_t>(numSets) || wayIndex >= static_cast<uint32_t>(associativity)) {
        return false;
    }
    return sets[setIndex].blocks[wayIndex].dirty;
}

uint32_t Cache::getBlockTag(uint32_t setIndex, uint32_t wayIndex) const {
    if (setIndex >= static_cast<uint32_t>(numSets) || wayIndex >= static_cast<uint32_t>(associativity)) {
        return 0;
    }
    return sets[setIndex].blocks[wayIndex].tag;
}

uint32_t Cache::getBlockAccessCount(uint32_t setIndex, uint32_t wayIndex) const {
    if (setIndex >= static_cast<uint32_t>(numSets) || wayIndex >= static_cast<uint32_t>(associativity)) {
        return 0;
    }
    return sets[setIndex].blocks[wayIndex].accessCount;
}

uint64_t Cache::getBlockLastAccess(uint32_t setIndex, uint32_t wayIndex) const {
    if (setIndex >= static_cast<uint32_t>(numSets) || wayIndex >= static_cast<uint32_t>(associativity)) {
        return 0;
    }
    return sets[setIndex].blocks[wayIndex].lastAccess;
}

bool Cache::isBlockPrefetched(uint32_t setIndex, uint32_t wayIndex) const {
    if (setIndex >= static_cast<uint32_t>(numSets) || wayIndex >= static_cast<uint32_t>(associativity)) {
        return false;
    }
    return sets[setIndex].blocks[wayIndex].prefetched;
}

MESIState Cache::getBlockState(uint32_t address) const {
    auto [tag, setIndex] = getTagAndSet(address);
    const auto& set = sets[setIndex];
    
    // Find the block in the set
    for (int i = 0; i < associativity; ++i) {
        const auto& block = set.blocks[i];
        if (block.valid && block.tag == tag) {
            return block.mesiState;
        }
    }
    
    return MESIState::Invalid;
}

} // namespace cachesim
