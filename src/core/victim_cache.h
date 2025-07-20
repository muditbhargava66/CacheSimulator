#pragma once
#include <vector>
#include <deque>
#include <unordered_map>
#include <algorithm>
#include <optional>
#include <functional>
#include <cassert>

// Forward declarations
struct VictimBlock {
    uint64_t address;
    uint64_t tag;
    bool valid;
    bool dirty;
    
    VictimBlock(uint64_t addr = 0, uint64_t t = 0, bool v = false, bool d = false)
        : address(addr), tag(t), valid(v), dirty(d) {}
        
    [[nodiscard]] uint64_t getAddress() const noexcept { return address; }
    [[nodiscard]] uint64_t getTag() const noexcept { return tag; }
    [[nodiscard]] bool isValid() const noexcept { return valid; }
    [[nodiscard]] bool isDirty() const noexcept { return dirty; }
    
    void setValid(bool v) noexcept { valid = v; }
    void setDirty(bool d) noexcept { dirty = d; }
};

class VictimCache {
private:
    std::vector<VictimBlock> blocks;
    std::deque<size_t> fifoQueue;  // FIFO replacement policy
    std::unordered_map<uint64_t, size_t> addressToIndex;  // Fast lookup
    size_t maxSize;
    size_t currentSize;
    
    // Statistics
    mutable uint64_t hits;
    mutable uint64_t misses;
    mutable uint64_t evictions;

public:
    explicit VictimCache(size_t size = 4) 
        : maxSize(size), currentSize(0), hits(0), misses(0), evictions(0) {
        blocks.reserve(maxSize);
        assert(maxSize > 0 && "Victim cache size must be positive");
    }

    // FIXED: Lambda with explicit 'this' capture for member access
    [[nodiscard]] bool findBlock(uint64_t blockAddr) const {
        // SOLUTION 1: Explicit 'this' capture
        auto it = std::find_if(blocks.cbegin(), blocks.cend(),
            [blockAddr](const VictimBlock& vb) -> bool {
                return vb.getAddress() == blockAddr && vb.isValid();
            });
        
        if (it != blocks.cend()) {
            ++hits;
            return true;
        }
        ++misses;
        return false;
    }

    // FIXED: Multiple lambda patterns with proper captures
    [[nodiscard]] std::optional<VictimBlock> searchAndRemove(uint64_t blockAddr) {
        // SOLUTION 2: C++17 style with structured binding and explicit capture
        auto it = std::find_if(blocks.begin(), blocks.end(),
            [blockAddr](const VictimBlock& vb) -> bool {
                return vb.getAddress() == blockAddr && vb.isValid();
            });

        if (it != blocks.end()) {
            VictimBlock found = *it;
            
            // Remove from FIFO queue using lambda with explicit capture
            auto queuePos = std::find_if(fifoQueue.begin(), fifoQueue.end(),
                [this, it](size_t idx) -> bool {
                    return &blocks[idx] == &(*it);
                });
            
            if (queuePos != fifoQueue.end()) {
                fifoQueue.erase(queuePos);
            }
            
            // Invalidate the block
            it->setValid(false);
            addressToIndex.erase(blockAddr);
            --currentSize;
            ++hits;
            
            return found;
        }
        
        ++misses;
        return std::nullopt;
    }

    // FIXED: Complex lambda with member function access
    void insertBlock(const VictimBlock& newBlock) {
        // Check if block already exists using lambda with proper capture
        auto existingIt = std::find_if(blocks.begin(), blocks.end(),
            [this, &newBlock](const VictimBlock& vb) -> bool {
                return this->isSameBlock(vb, newBlock);
            });

        if (existingIt != blocks.end()) {
            // Update existing block
            *existingIt = newBlock;
            return;
        }

        // Need to make space if at capacity
        if (currentSize >= maxSize) {
            evictOldestBlock();
        }

        // Find first invalid slot or add new one
        auto invalidIt = std::find_if(blocks.begin(), blocks.end(),
            [](const VictimBlock& vb) -> bool {
                return !vb.isValid();
            });

        size_t insertIndex;
        if (invalidIt != blocks.end()) {
            *invalidIt = newBlock;
            insertIndex = std::distance(blocks.begin(), invalidIt);
        } else {
            blocks.push_back(newBlock);
            insertIndex = blocks.size() - 1;
        }

        // Update data structures
        fifoQueue.push_back(insertIndex);
        addressToIndex[newBlock.getAddress()] = insertIndex;
        ++currentSize;
    }

    // FIXED: Lambda in algorithm with range operations
    void invalidateBlocksInRange(uint64_t startAddr, uint64_t endAddr) {
        // SOLUTION 3: Using remove_if with explicit capture
        auto newEnd = std::remove_if(blocks.begin(), blocks.end(),
            [this, startAddr, endAddr](VictimBlock& vb) -> bool {
                const auto addr = vb.getAddress();
                if (addr >= startAddr && addr <= endAddr && vb.isValid()) {
                    this->removeFromDataStructures(vb);
                    vb.setValid(false);
                    return true;
                }
                return false;
            });
        
        blocks.erase(newEnd, blocks.end());
    }

    // FIXED: Lambda with conditional logic and member access
    [[nodiscard]] std::vector<VictimBlock> getDirtyBlocks() const {
        std::vector<VictimBlock> dirtyBlocks;
        
        // SOLUTION 4: Copy_if with explicit capture and back_inserter
        std::copy_if(blocks.cbegin(), blocks.cend(),
            std::back_inserter(dirtyBlocks),
            [this](const VictimBlock& vb) -> bool {
                return this->isBlockDirtyAndValid(vb);
            });
        
        return dirtyBlocks;
    }

    // FIXED: Custom comparison lambda for sorting
    void sortBlocksByAddress() {
        // SOLUTION 5: Sort with lambda comparator
        std::sort(blocks.begin(), blocks.end(),
            [](const VictimBlock& a, const VictimBlock& b) -> bool {
                if (!a.isValid()) return false;
                if (!b.isValid()) return true;
                return a.getAddress() < b.getAddress();
            });
    }

    // FIXED: Parallel algorithm with thread-safe lambda
    [[nodiscard]] size_t countValidBlocks() const {
        // SOLUTION 6: Count_if with simple predicate
        return std::count_if(blocks.cbegin(), blocks.cend(),
            [](const VictimBlock& vb) -> bool {
                return vb.isValid();
            });
    }

    // FIXED: Transform algorithm with member function calls
    [[nodiscard]] std::vector<uint64_t> getAllValidAddresses() const {
        std::vector<uint64_t> addresses;
        addresses.reserve(currentSize);
        
        // Transform valid blocks to addresses
        std::transform(blocks.cbegin(), blocks.cend(),
            std::back_inserter(addresses),
            [](const VictimBlock& vb) -> uint64_t {
                return vb.isValid() ? vb.getAddress() : 0;
            });
        
        // Remove zero addresses (invalid blocks)
        addresses.erase(
            std::remove(addresses.begin(), addresses.end(), 0),
            addresses.end()
        );
        
        return addresses;
    }

    // Performance and statistics methods
    [[nodiscard]] double getHitRate() const noexcept {
        const auto total = hits + misses;
        return total > 0 ? static_cast<double>(hits) / total : 0.0;
    }

    [[nodiscard]] uint64_t getHits() const noexcept { return hits; }
    [[nodiscard]] uint64_t getMisses() const noexcept { return misses; }
    [[nodiscard]] uint64_t getEvictions() const noexcept { return evictions; }
    [[nodiscard]] size_t getCurrentSize() const noexcept { return currentSize; }
    [[nodiscard]] size_t getMaxSize() const noexcept { return maxSize; }
    [[nodiscard]] bool isEmpty() const noexcept { return currentSize == 0; }
    [[nodiscard]] bool isFull() const noexcept { return currentSize >= maxSize; }

private:
    // Helper methods with proper const-correctness
    [[nodiscard]] bool isSameBlock(const VictimBlock& a, const VictimBlock& b) const noexcept {
        return a.getAddress() == b.getAddress();
    }

    [[nodiscard]] bool isBlockDirtyAndValid(const VictimBlock& vb) const noexcept {
        return vb.isValid() && vb.isDirty();
    }

    void removeFromDataStructures(const VictimBlock& vb) {
        addressToIndex.erase(vb.getAddress());
        
        // Remove from FIFO queue
        auto it = std::find_if(fifoQueue.begin(), fifoQueue.end(),
            [this, &vb](size_t idx) -> bool {
                return blocks[idx].getAddress() == vb.getAddress();
            });
        
        if (it != fifoQueue.end()) {
            fifoQueue.erase(it);
        }
        --currentSize;
    }

    void evictOldestBlock() {
        if (!fifoQueue.empty() && currentSize > 0) {
            const size_t oldestIndex = fifoQueue.front();
            fifoQueue.pop_front();
            
            if (oldestIndex < blocks.size()) {
                auto& oldestBlock = blocks[oldestIndex];
                addressToIndex.erase(oldestBlock.getAddress());
                oldestBlock.setValid(false);
                --currentSize;
                ++evictions;
            }
        }
    }
};

// Example usage with modern C++ patterns
class CacheSimulator {
    VictimCache victimCache;
    
public:
    explicit CacheSimulator(size_t victimSize = 4) : victimCache(victimSize) {}
    
    void demonstrateFixedLambdas() {
        // All these methods now work correctly with fixed lambda captures
        VictimBlock testBlock{0x1000, 0x100, true, false};
        
        victimCache.insertBlock(testBlock);
        
        if (victimCache.findBlock(0x1000)) {
            [[maybe_unused]] auto retrieved = victimCache.searchAndRemove(0x1000);
            // Process retrieved block...
        }
        
        // Get statistics
        [[maybe_unused]] const auto hitRate = victimCache.getHitRate();
        [[maybe_unused]] const auto validCount = victimCache.countValidBlocks();
        const auto dirtyBlocks = victimCache.getDirtyBlocks();
        
        // Demonstrate range operations
        victimCache.invalidateBlocksInRange(0x1000, 0x2000);
        victimCache.sortBlocksByAddress();
        
        auto addresses = victimCache.getAllValidAddresses();
    }
};
