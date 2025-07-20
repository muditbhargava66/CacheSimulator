/**
 * @file cache.h
 * @brief Cache implementation with configurable parameters and advanced features
 * @author Mudit Bhargava
 * @date 2025-05-27
 * @version 1.1.0
 *
 * This file defines the Cache class which implements a configurable cache
 * with support for various replacement policies, prefetching strategies,
 * and cache coherence protocols.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>
#include <stdexcept>
#include <utility>
#include <random>

#include "cache_block.h"
#include "mesi_protocol.h"
#include "stride_predictor.h"
#include "stream_buffer.h"
#include "replacement_policy.h"

namespace cachesim {

/**
 * @enum CacheMissType
 * @brief Types of cache misses for detailed statistics tracking
 */
enum class CacheMissType {
    Compulsory,  ///< First access to this block (cold miss)
    Capacity,    ///< Cache is full, had to evict (capacity miss)
    Conflict,    ///< Set is full, had to evict (conflict miss)
    Coherence    ///< Invalidated due to coherence protocol
};

/**
 * @brief String representations of cache miss types for display purposes
 */
inline const std::array<std::string_view, 4> CacheMissTypeNames = {
    "Compulsory", "Capacity", "Conflict", "Coherence"
};

/**
 * @enum WritePolicy  
 * @brief Cache write policies (v1.1.0)
 */
enum class WritePolicy {
    WriteBack,      ///< Write-back (default) - write to cache, mark dirty
    WriteThrough    ///< Write-through - write to both cache and memory
};

// Exception class for cache configuration errors
class CacheConfigError : public std::runtime_error {
public:
    explicit CacheConfigError(const std::string& message) 
        : std::runtime_error("Cache configuration error: " + message) {}
};

// Cache configuration structure using C++17's structured bindings capability
struct CacheConfig {
    int size;            // Cache size in bytes
    int associativity;   // Number of ways in each set
    int blockSize;       // Size of each cache block in bytes
    bool prefetchEnabled; // Is prefetching enabled?
    int prefetchDistance; // Prefetch distance
    ReplacementPolicy replacementPolicy = ReplacementPolicy::LRU; // v1.1.0
    WritePolicy writePolicy = WritePolicy::WriteBack; // v1.1.0
    
    // Default constructor using C++17 inline initialization
    CacheConfig() = default;
    
    // Constructor with parameters and validation
    CacheConfig(int size, int associativity, int blockSize, 
                bool prefetchEnabled = false, int prefetchDistance = 0,
                ReplacementPolicy replacementPolicy = ReplacementPolicy::LRU,
                WritePolicy writePolicy = WritePolicy::WriteBack)
        : size(size), associativity(associativity), blockSize(blockSize),
          prefetchEnabled(prefetchEnabled), prefetchDistance(prefetchDistance),
          replacementPolicy(replacementPolicy), writePolicy(writePolicy) {
        validate();
    }
    
    // Validation method
    void validate() const {
        if (size <= 0) {
            throw CacheConfigError("Cache size must be positive");
        }
        if (associativity <= 0) {
            throw CacheConfigError("Associativity must be positive");
        }
        if (blockSize <= 0 || (blockSize & (blockSize - 1)) != 0) {
            throw CacheConfigError("Block size must be a positive power of 2");
        }
        if (size % (associativity * blockSize) != 0) {
            throw CacheConfigError("Cache size must be divisible by (associativity * blockSize)");
        }
        if (prefetchDistance < 0) {
            throw CacheConfigError("Prefetch distance must be non-negative");
        }
    }
};

// Forward declaration of related classes
class MemoryHierarchy;

/**
 * @class Cache
 * @brief Models a single cache level in the memory hierarchy
 *
 * The Cache class implements a configurable cache with support for:
 * - Various replacement policies (currently LRU)
 * - Prefetching mechanisms (stream buffer and stride-based)
 * - MESI coherence protocol
 * - Detailed statistics tracking
 * - Cache state export and warmup functionality
 *
 * The cache uses a set-associative organization with configurable
 * size, associativity, and block size. All parameters must be
 * validated through the CacheConfig structure.
 */
class Cache {
public:
    // Constructor with C++17's explicit keyword and nodiscard attribute
    [[nodiscard]] explicit Cache(const CacheConfig& config);
    
    // Access method returns true for hit, false for miss
    [[nodiscard]] bool access(uint32_t address, bool isWrite, 
                             std::optional<Cache*> nextLevel = std::nullopt,
                             std::optional<StridePredictor*> stridePredictor = std::nullopt);
    
    // Coherence operations
    void invalidateBlock(uint32_t address);
    void updateMESIState(uint32_t address, MESIState newState);
    
    // Statistics methods
    void printStats() const;
    void resetStats();
    
    // Helper methods
    [[nodiscard]] std::pair<uint32_t, int> getTagAndSet(uint32_t address) const;
    [[nodiscard]] int getBlocksPerSet() const { return associativity; }
    [[nodiscard]] int getNumSets() const { return numSets; }
    
    // Getters for statistics
    [[nodiscard]] int getHits() const { return hits; }
    [[nodiscard]] int getMisses() const { return misses; }
    [[nodiscard]] int getReads() const { return reads; }
    [[nodiscard]] int getWrites() const { return writes; }
    [[nodiscard]] int getWriteBacks() const { return writeBacks; }
    [[nodiscard]] const auto& getMissTypeStats() const { return missTypeStats; }
    
    // Returns miss ratio as double with C++17's [[nodiscard]]
    [[nodiscard]] double getMissRatio() const {
        const int total = hits + misses;
        return total > 0 ? static_cast<double>(misses) / total : 0.0;
    }
    
    // Additional efficiency metrics (v1.1.0)
    [[nodiscard]] double getHitRatio() const {
        const int total = hits + misses;
        return total > 0 ? static_cast<double>(hits) / total : 0.0;
    }
    
    [[nodiscard]] double getWriteBackRatio() const {
        const int total = hits + misses;
        return total > 0 ? static_cast<double>(writeBacks) / total : 0.0;
    }
    
    [[nodiscard]] double getCacheEfficiency() const {
        // Efficiency metric combining hit rate and writeback overhead
        return getHitRatio() * (1.0 - getWriteBackRatio() * 0.1);
    }
    
    // Export cache state for debugging (v1.1.0)
    [[nodiscard]] std::string exportCacheState() const;
    
    // Cache warmup for benchmarking (v1.1.0)
    void warmup(const std::vector<uint32_t>& addresses);
    
    // Block state access methods for visualization (v1.2.0)
    [[nodiscard]] bool isBlockValid(uint32_t setIndex, uint32_t wayIndex) const;
    [[nodiscard]] bool isBlockDirty(uint32_t setIndex, uint32_t wayIndex) const;
    [[nodiscard]] uint32_t getBlockTag(uint32_t setIndex, uint32_t wayIndex) const;
    [[nodiscard]] uint32_t getBlockAccessCount(uint32_t setIndex, uint32_t wayIndex) const;
    [[nodiscard]] uint64_t getBlockLastAccess(uint32_t setIndex, uint32_t wayIndex) const;
    [[nodiscard]] bool isBlockPrefetched(uint32_t setIndex, uint32_t wayIndex) const;
    [[nodiscard]] uint32_t getAssociativity() const { return associativity; }
    [[nodiscard]] uint32_t getBlockSize() const { return blockSize; }
    
    // Get MESI state of a block
    [[nodiscard]] MESIState getBlockState(uint32_t address) const;

private:
    // Cache parameters
    int size;
    int associativity;
    int blockSize;
    int numSets;
    ReplacementPolicy replacementPolicy; // v1.1.0
    WritePolicy writePolicy; // v1.1.0
    
    // Cache data structures
    std::vector<CacheSet> sets;
    std::vector<std::unique_ptr<ReplacementPolicyBase>> replacementPolicies; // v1.1.0
    
    // Prefetching support
    StreamBuffer streamBuffer;
    bool prefetchEnabled;
    
    // MESI protocol support
    MESIProtocol mesiProtocol;
    
    // Random number generator for Random replacement policy (v1.1.0)
    mutable std::mt19937 rng{std::random_device{}()};
    
    // Statistics
    int hits;
    int misses;
    int reads;
    int writes;
    int writeBacks;
    int writeThroughs; // v1.1.0 - count of write-through operations
    std::array<int, 4> missTypeStats; // Counts for each type of miss
    
    // Private cache operations
    void writeBack(uint32_t address, Cache* nextLevel);
    void writeThrough(uint32_t address, Cache* nextLevel); // v1.1.0
    
    // Block fetching and eviction
    int findVictim(const CacheSet& set, int setIndex) const; // v1.1.0 - added setIndex
    void installBlock(uint32_t address, int setIndex, int blockIndex, bool isWrite, Cache* nextLevel, bool isPrefetch = false);
    
    // C++17 variant to represent either a hit or miss
    using AccessResult = std::variant<int, CacheMissType>; // hit: block index, miss: type
    
    // Check if block is present in cache
    AccessResult findBlock(uint32_t address) const;
    
    // Prefetching logic
    void handlePrefetch(uint32_t address, bool isWrite, Cache* nextLevel, StridePredictor* stridePredictor);
};

} // namespace cachesim