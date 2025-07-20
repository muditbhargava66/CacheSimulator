/**
 * @file replacement_policy.h
 * @brief Cache replacement policy interface and implementations
 * @author Mudit Bhargava
 * @date 2025-05-28
 * @version 1.1.0
 *
 * This file defines the interface for cache replacement policies and provides
 * implementations for various policies including LRU, FIFO, Random, and PLRU.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <random>
#include <memory>
#include <algorithm>
#include <string_view>

namespace cachesim {

/**
 * @enum ReplacementPolicy
 * @brief Enumeration of available replacement policies
 */
enum class ReplacementPolicy {
    LRU,    ///< Least Recently Used
    FIFO,   ///< First In First Out
    Random, ///< Random replacement
    PLRU,   ///< Pseudo-LRU (tree-based)
    NRU     ///< Not Recently Used
};

/**
 * @brief Convert replacement policy enum to string
 */
inline constexpr std::string_view replacementPolicyToString(ReplacementPolicy policy) {
    switch (policy) {
        case ReplacementPolicy::LRU:    return "LRU";
        case ReplacementPolicy::FIFO:   return "FIFO";
        case ReplacementPolicy::Random: return "Random";
        case ReplacementPolicy::PLRU:   return "PLRU";
        case ReplacementPolicy::NRU:    return "NRU";
        default:                        return "Unknown";
    }
}

/**
 * @class ReplacementPolicyBase
 * @brief Abstract base class for cache replacement policies
 */
class ReplacementPolicyBase {
public:
    virtual ~ReplacementPolicyBase() = default;
    
    /**
     * @brief Update policy state on cache access
     * @param blockIndex Index of accessed block
     */
    virtual void onAccess(int blockIndex) = 0;
    
    /**
     * @brief Update policy state on block installation
     * @param blockIndex Index of installed block
     */
    virtual void onInstall(int blockIndex) = 0;
    
    /**
     * @brief Select victim block for replacement
     * @param validBlocks Bit vector indicating which blocks are valid
     * @return Index of victim block
     */
    virtual int selectVictim(const std::vector<bool>& validBlocks) = 0;
    
    /**
     * @brief Reset policy state
     */
    virtual void reset() = 0;
    
    /**
     * @brief Get policy name
     * @return Policy name as string view
     */
    virtual std::string_view getName() const = 0;
};

/**
 * @class LRUPolicy
 * @brief Least Recently Used replacement policy
 */
class LRUPolicy : public ReplacementPolicyBase {
public:
    explicit LRUPolicy(int numBlocks) 
        : numBlocks_(numBlocks), lruOrder_(numBlocks) {
        reset();
    }
    
    void onAccess(int blockIndex) override {
        // Move accessed block to front (most recently used)
        auto it = std::find(lruOrder_.begin(), lruOrder_.end(), blockIndex);
        if (it != lruOrder_.end()) {
            lruOrder_.erase(it);
            lruOrder_.insert(lruOrder_.begin(), blockIndex);
        }
    }
    
    void onInstall(int blockIndex) override {
        // Same as access for LRU
        onAccess(blockIndex);
    }
    
    int selectVictim(const std::vector<bool>& validBlocks) override {
        // Return least recently used valid block
        for (auto it = lruOrder_.rbegin(); it != lruOrder_.rend(); ++it) {
            if (static_cast<size_t>(*it) < validBlocks.size() && validBlocks[*it]) {
                return *it;
            }
        }
        // Fallback: return first valid block
        for (size_t i = 0; i < validBlocks.size(); ++i) {
            if (validBlocks[i]) return static_cast<int>(i);
        }
        return 0;
    }
    
    void reset() override {
        lruOrder_.resize(numBlocks_);
        std::iota(lruOrder_.begin(), lruOrder_.end(), 0);
    }
    
    std::string_view getName() const override { return "LRU"; }
    
private:
    int numBlocks_;
    std::vector<int> lruOrder_; ///< Blocks ordered from MRU to LRU
};

/**
 * @class FIFOPolicy
 * @brief First In First Out replacement policy
 */
class FIFOPolicy : public ReplacementPolicyBase {
public:
    explicit FIFOPolicy(int numBlocks) 
        : fifoOrder_(numBlocks) {
        reset();
    }
    
    void onAccess(int blockIndex) override {
        // FIFO doesn't change on access
        (void)blockIndex;
    }
    
    void onInstall(int blockIndex) override {
        // Record installation order
        fifoOrder_[blockIndex] = installCounter_++;
    }
    
    int selectVictim(const std::vector<bool>& validBlocks) override {
        // Return oldest valid block
        int victim = -1;
        uint64_t oldestTime = UINT64_MAX;
        
        for (size_t i = 0; i < validBlocks.size(); ++i) {
            if (validBlocks[i] && fifoOrder_[i] < oldestTime) {
                oldestTime = fifoOrder_[i];
                victim = static_cast<int>(i);
            }
        }
        
        return (victim >= 0) ? victim : 0;
    }
    
    void reset() override {
        std::fill(fifoOrder_.begin(), fifoOrder_.end(), 0);
        installCounter_ = 0;
    }
    
    std::string_view getName() const override { return "FIFO"; }
    
private:
    std::vector<uint64_t> fifoOrder_; ///< Installation timestamp for each block
    uint64_t installCounter_ = 0;      ///< Counter for installation order
};

/**
 * @class RandomPolicy
 * @brief Random replacement policy
 */
class RandomPolicy : public ReplacementPolicyBase {
public:
    explicit RandomPolicy(int /* numBlocks */) 
        : rng_(std::random_device{}()) {}
    
    void onAccess(int blockIndex) override {
        // Random doesn't track accesses
        (void)blockIndex;
    }
    
    void onInstall(int blockIndex) override {
        // Random doesn't track installations
        (void)blockIndex;
    }
    
    int selectVictim(const std::vector<bool>& validBlocks) override {
        // Collect valid block indices
        std::vector<int> validIndices;
        for (size_t i = 0; i < validBlocks.size(); ++i) {
            if (validBlocks[i]) {
                validIndices.push_back(static_cast<int>(i));
            }
        }
        
        if (validIndices.empty()) return 0;
        
        // Select random valid block
        std::uniform_int_distribution<int> dist(0, validIndices.size() - 1);
        return validIndices[dist(rng_)];
    }
    
    void reset() override {
        // Nothing to reset for random policy
    }
    
    std::string_view getName() const override { return "Random"; }
    
private:
    mutable std::mt19937 rng_; ///< Random number generator
};

/**
 * @class NRUPolicy
 * @brief Not Recently Used replacement policy
 * 
 * Uses reference bits to track recent usage. Periodically clears
 * reference bits to give all blocks a chance to be marked as used.
 */
class NRUPolicy : public ReplacementPolicyBase {
public:
    explicit NRUPolicy(int numBlocks) 
        : referenceBits_(numBlocks, false), 
          accessCounter_(0), clearInterval_(numBlocks * 4) {}
    
    void onAccess(int blockIndex) override {
        referenceBits_[blockIndex] = true;
        accessCounter_++;
        
        // Periodically clear all reference bits
        if (accessCounter_ >= clearInterval_) {
            std::fill(referenceBits_.begin(), referenceBits_.end(), false);
            accessCounter_ = 0;
        }
    }
    
    void onInstall(int blockIndex) override {
        referenceBits_[blockIndex] = true;
    }
    
    int selectVictim(const std::vector<bool>& validBlocks) override {
        // First, try to find a valid block that is not recently used
        for (size_t i = 0; i < validBlocks.size(); ++i) {
            if (validBlocks[i] && !referenceBits_[i]) {
                return static_cast<int>(i);
            }
        }
        
        // If all blocks are recently used, clear reference bits and select first valid
        std::fill(referenceBits_.begin(), referenceBits_.end(), false);
        accessCounter_ = 0;
        
        for (size_t i = 0; i < validBlocks.size(); ++i) {
            if (validBlocks[i]) return static_cast<int>(i);
        }
        return 0;
    }
    
    void reset() override {
        std::fill(referenceBits_.begin(), referenceBits_.end(), false);
        accessCounter_ = 0;
    }
    
    std::string_view getName() const override { return "NRU"; }
    
private:
    std::vector<bool> referenceBits_;  ///< Track if block was recently used
    int accessCounter_;                ///< Count accesses for periodic clearing
    int clearInterval_;                ///< How often to clear reference bits
};

/**
 * @class PLRUPolicy
 * @brief Pseudo-LRU replacement policy using binary tree
 */
class PLRUPolicy : public ReplacementPolicyBase {
public:
    explicit PLRUPolicy(int numBlocks) 
        : numBlocks_(numBlocks), treeBits_(numBlocks - 1, false) {
        // Tree bits for binary tree structure
        // For n blocks, need n-1 bits
    }
    
    void onAccess(int blockIndex) override {
        // Update tree bits based on access
        updateTreeBits(blockIndex, true);
    }
    
    void onInstall(int blockIndex) override {
        // Same as access for PLRU
        onAccess(blockIndex);
    }
    
    int selectVictim(const std::vector<bool>& validBlocks) override {
        // Follow tree bits to find victim
        int node = 0;
        int numLevels = static_cast<int>(std::log2(numBlocks_));
        
        for (int i = 0; i < numLevels; ++i) {
            if (static_cast<size_t>(node) >= treeBits_.size()) break;
            
            // Go left (0) or right (1) based on tree bit
            if (treeBits_[node]) {
                node = 2 * node + 2; // Right child
            } else {
                node = 2 * node + 1; // Left child
            }
        }
        
        // Calculate block index from final node
        int blockIndex = node - (numBlocks_ - 1);
        
        // Ensure we return a valid block
        if (static_cast<size_t>(blockIndex) < validBlocks.size() && validBlocks[blockIndex]) {
            return blockIndex;
        }
        
        // Fallback: return first valid block
        for (size_t i = 0; i < validBlocks.size(); ++i) {
            if (validBlocks[i]) return i;
        }
        return 0;
    }
    
    void reset() override {
        std::fill(treeBits_.begin(), treeBits_.end(), false);
    }
    
    std::string_view getName() const override { return "PLRU"; }
    
private:
    void updateTreeBits(int blockIndex, bool /* accessed */) {
        // Update tree bits to point away from accessed block
        int node = blockIndex + (numBlocks_ - 1);
        
        while (node > 0) {
            int parent = (node - 1) / 2;
            // Set bit to point to opposite child
            treeBits_[parent] = (node % 2 == 0);
            node = parent;
        }
    }
    
    int numBlocks_;
    std::vector<bool> treeBits_; ///< Binary tree for PLRU
};

/**
 * @class ReplacementPolicyFactory
 * @brief Factory for creating replacement policy instances
 */
class ReplacementPolicyFactory {
public:
    /**
     * @brief Create a replacement policy instance
     * @param policy Policy type to create
     * @param numBlocks Number of blocks in the cache set
     * @return Unique pointer to the created policy
     */
    static std::unique_ptr<ReplacementPolicyBase> create(
        ReplacementPolicy policy, int numBlocks) {
        
        switch (policy) {
            case ReplacementPolicy::LRU:
                return std::make_unique<LRUPolicy>(numBlocks);
            case ReplacementPolicy::FIFO:
                return std::make_unique<FIFOPolicy>(numBlocks);
            case ReplacementPolicy::Random:
                return std::make_unique<RandomPolicy>(numBlocks);
            case ReplacementPolicy::PLRU:
                return std::make_unique<PLRUPolicy>(numBlocks);
            case ReplacementPolicy::NRU:
                return std::make_unique<NRUPolicy>(numBlocks);
            default:
                // Default to LRU
                return std::make_unique<LRUPolicy>(numBlocks);
        }
    }
};

} // namespace cachesim
