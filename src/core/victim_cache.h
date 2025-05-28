/**
 * @file victim_cache.h
 * @brief Victim cache implementation for the cache simulator
 * @author Mudit Bhargava
 * @date 2025-05-29
 * @version 1.2.0
 *
 * This file implements a victim cache which stores recently evicted blocks
 * from the main cache to reduce conflict misses.
 */

#pragma once

#include "cache_block.h"
#include <vector>
#include <deque>
#include <unordered_map>
#include <cstdint>
#include <optional>
#include <algorithm>

namespace cachesim {

/**
 * @class VictimCache
 * @brief Fully associative cache for storing evicted blocks
 * 
 * The victim cache stores blocks that have been evicted from the main cache.
 * It acts as a small, fully-associative buffer between cache levels to
 * reduce the penalty of conflict misses.
 */
class VictimCache {
public:
    struct VictimBlock {
        CacheBlock block;
        uint32_t fullAddress;  ///< Full address including tag, set, and offset
        uint64_t accessTime;   ///< Last access time for LRU
    };
    
    struct Statistics {
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t insertions = 0;
        uint64_t evictions = 0;
        uint64_t writebacks = 0;
        
        double getHitRate() const {
            uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / total : 0.0;
        }
    };
    
    /**
     * @brief Constructor
     * @param numEntries Number of entries in the victim cache (typically 4-16)
     * @param blockSize Size of each cache block in bytes
     */
    explicit VictimCache(int numEntries = 8, int blockSize = 64)
        : numEntries_(numEntries), blockSize_(blockSize), currentTime_(0) {
        entries_.reserve(numEntries);
    }
    
    /**
     * @brief Look up an address in the victim cache
     * @param address Memory address to look up
     * @return Optional containing the block if found
     */
    std::optional<CacheBlock> lookup(uint32_t address) {
        uint32_t blockAddr = getBlockAddress(address);
        
        auto it = std::find_if(entries_.begin(), entries_.end(),
            [blockAddr](const VictimBlock& vb) {
                return getBlockAddress(vb.fullAddress) == blockAddr;
            });
        
        if (it != entries_.end()) {
            // Hit - update access time and statistics
            it->accessTime = currentTime_++;
            stats_.hits++;
            
            // Move to front for better cache locality
            if (it != entries_.begin()) {
                std::rotate(entries_.begin(), it, it + 1);
            }
            
            return it->block;
        }
        
        // Miss
        stats_.misses++;
        return std::nullopt;
    }
    
    /**
     * @brief Insert a block into the victim cache
     * @param address Full address of the block
     * @param block The cache block to insert
     * @return Optional containing evicted block if victim cache was full
     */
    std::optional<VictimBlock> insert(uint32_t address, const CacheBlock& block) {
        std::optional<VictimBlock> evicted;
        
        // Check if block already exists (shouldn't happen in correct usage)
        uint32_t blockAddr = getBlockAddress(address);
        auto it = std::find_if(entries_.begin(), entries_.end(),
            [blockAddr](const VictimBlock& vb) {
                return getBlockAddress(vb.fullAddress) == blockAddr;
            });
        
        if (it != entries_.end()) {
            // Update existing entry
            it->block = block;
            it->accessTime = currentTime_++;
            return evicted;
        }
        
        // Insert new entry
        if (entries_.size() >= numEntries_) {
            // Find LRU victim
            auto lruIt = std::min_element(entries_.begin(), entries_.end(),
                [](const VictimBlock& a, const VictimBlock& b) {
                    return a.accessTime < b.accessTime;
                });
            
            evicted = *lruIt;
            stats_.evictions++;
            
            if (evicted->block.dirty) {
                stats_.writebacks++;
            }
            
            // Replace LRU entry
            *lruIt = VictimBlock{block, address, currentTime_++};
        } else {
            // Space available, just insert
            entries_.push_back(VictimBlock{block, address, currentTime_++});
        }
        
        stats_.insertions++;
        return evicted;
    }
    
    /**
     * @brief Remove a block from the victim cache
     * @param address Address of the block to remove
     * @return True if block was found and removed
     */
    bool remove(uint32_t address) {
        uint32_t blockAddr = getBlockAddress(address);
        
        auto it = std::find_if(entries_.begin(), entries_.end(),
            [blockAddr](const VictimBlock& vb) {
                return getBlockAddress(vb.fullAddress) == blockAddr;
            });
        
        if (it != entries_.end()) {
            entries_.erase(it);
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief Clear all entries in the victim cache
     */
    void clear() {
        entries_.clear();
        currentTime_ = 0;
    }
    
    /**
     * @brief Get current statistics
     */
    const Statistics& getStatistics() const { return stats_; }
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics() {
        stats_ = Statistics{};
    }
    
    /**
     * @brief Get current occupancy
     */
    size_t getOccupancy() const { return entries_.size(); }
    
    /**
     * @brief Get maximum capacity
     */
    size_t getCapacity() const { return numEntries_; }
    
    /**
     * @brief Check if victim cache is full
     */
    bool isFull() const { return entries_.size() >= numEntries_; }
    
    /**
     * @brief Get all entries for debugging
     */
    const std::vector<VictimBlock>& getEntries() const { return entries_; }
    
private:
    /**
     * @brief Extract block address from full address
     */
    uint32_t getBlockAddress(uint32_t address) const {
        return address & ~(blockSize_ - 1);
    }
    
    int numEntries_;              ///< Maximum number of entries
    int blockSize_;               ///< Size of each block in bytes
    uint64_t currentTime_;        ///< Current time for LRU tracking
    std::vector<VictimBlock> entries_;  ///< Victim cache entries
    Statistics stats_;            ///< Performance statistics
};

/**
 * @class VictimCacheConfig
 * @brief Configuration for victim cache
 */
struct VictimCacheConfig {
    int numEntries = 8;       ///< Number of victim cache entries
    int blockSize = 64;       ///< Block size in bytes
    bool enabled = false;     ///< Whether victim cache is enabled
    
    /**
     * @brief Validate configuration
     * @throw std::invalid_argument if configuration is invalid
     */
    void validate() const {
        if (enabled) {
            if (numEntries <= 0 || numEntries > 64) {
                throw std::invalid_argument("Victim cache entries must be between 1 and 64");
            }
            if (blockSize <= 0 || (blockSize & (blockSize - 1)) != 0) {
                throw std::invalid_argument("Victim cache block size must be a power of 2");
            }
        }
    }
};

} // namespace cachesim
