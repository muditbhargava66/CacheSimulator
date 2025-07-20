/**
 * @file write_policy.h
 * @brief Cache write policy implementations
 * @author Mudit Bhargava
 * @date 2025-05-29
 * @version 1.2.0
 *
 * This file implements different write policies for the cache simulator
 * including write-back and write-through policies with various allocation strategies.
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <vector>
#include <algorithm>

namespace cachesim {

// Forward declaration
class Cache;

/**
 * @enum WriteAllocationPolicy
 * @brief Write allocation policy on write miss
 */
enum class WriteAllocationPolicy {
    WriteAllocate,      ///< Allocate cache line on write miss (default)
    NoWriteAllocate     ///< Do not allocate cache line on write miss
};

/**
 * @enum WriteUpdatePolicy  
 * @brief Write update policy on write hit
 */
enum class WriteUpdatePolicy {
    WriteBack,          ///< Write only to cache, mark dirty
    WriteThrough        ///< Write to both cache and next level
};

/**
 * @class WritePolicyBase
 * @brief Abstract base class for write policies
 */
class WritePolicyBase {
public:
    virtual ~WritePolicyBase() = default;
    
    /**
     * @brief Handle a write operation
     * @param address Memory address being written
     * @param cache Current cache level
     * @param nextLevel Next level in memory hierarchy (optional)
     * @param hit Whether the write was a hit
     * @return Number of write operations performed
     */
    virtual int handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) = 0;
    
    /**
     * @brief Check if writeback is needed on eviction
     * @return True if dirty blocks need writeback
     */
    virtual bool requiresWriteback() const = 0;
    
    /**
     * @brief Check if allocation is needed on write miss
     * @return True if cache line should be allocated
     */
    virtual bool requiresAllocation() const = 0;
    
    /**
     * @brief Get policy name
     * @return Policy name as string view
     */
    virtual std::string_view getName() const = 0;
};

/**
 * @class WriteThroughPolicy
 * @brief Implements write-through with write-allocate policy
 */
class WriteThroughPolicy : public WritePolicyBase {
public:
    int handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) override;
    bool requiresWriteback() const override { return false; }
    bool requiresAllocation() const override { return true; }
    std::string_view getName() const override { return "WriteThrough+WriteAllocate"; }
};

/**
 * @class WriteThroughNoAllocatePolicy
 * @brief Implements write-through with no-write-allocate policy
 */
class WriteThroughNoAllocatePolicy : public WritePolicyBase {
public:
    int handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) override;
    bool requiresWriteback() const override { return false; }
    bool requiresAllocation() const override { return false; }
    std::string_view getName() const override { return "WriteThrough+NoWriteAllocate"; }
};

/**
 * @class WriteBackPolicy
 * @brief Implements write-back with write-allocate policy (default)
 */
class WriteBackPolicy : public WritePolicyBase {
public:
    int handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) override;
    bool requiresWriteback() const override { return true; }
    bool requiresAllocation() const override { return true; }
    std::string_view getName() const override { return "WriteBack+WriteAllocate"; }
};

/**
 * @class WriteBackNoAllocatePolicy
 * @brief Implements write-back with no-write-allocate policy
 */
class WriteBackNoAllocatePolicy : public WritePolicyBase {
public:
    int handleWrite(uint32_t address, Cache* cache, Cache* nextLevel, bool hit) override;
    bool requiresWriteback() const override { return true; }
    bool requiresAllocation() const override { return false; }
    std::string_view getName() const override { return "WriteBack+NoWriteAllocate"; }
};

/**
 * @class WriteCombiningBuffer
 * @brief Implements write combining buffer for coalescing writes
 */
class WriteCombiningBuffer {
public:
    static constexpr size_t DEFAULT_BUFFER_SIZE = 8;
    
    struct Entry {
        uint32_t address;
        bool valid;
        uint64_t timestamp;
    };
    
    explicit WriteCombiningBuffer(size_t size = DEFAULT_BUFFER_SIZE) 
        : entries_(size), currentTime_(0) {
        for (auto& entry : entries_) {
            entry.valid = false;
        }
    }
    
    /**
     * @brief Try to combine a write with existing buffer entries
     * @param address Write address
     * @return True if write was combined, false if buffer is full
     */
    bool tryWrite(uint32_t address) {
        currentTime_++;
        
        // Check if address already in buffer
        for (auto& entry : entries_) {
            if (entry.valid && entry.address == address) {
                entry.timestamp = currentTime_;
                return true;
            }
        }
        
        // Find empty slot
        for (auto& entry : entries_) {
            if (!entry.valid) {
                entry.address = address;
                entry.valid = true;
                entry.timestamp = currentTime_;
                return true;
            }
        }
        
        // Buffer full - evict oldest entry
        auto oldestIt = std::min_element(entries_.begin(), entries_.end(),
            [](const Entry& a, const Entry& b) {
                return a.timestamp < b.timestamp;
            });
        
        oldestIt->address = address;
        oldestIt->timestamp = currentTime_;
        return false; // Indicates eviction occurred
    }
    
    /**
     * @brief Flush all valid entries
     * @return Vector of addresses to write
     */
    std::vector<uint32_t> flush() {
        std::vector<uint32_t> addresses;
        for (auto& entry : entries_) {
            if (entry.valid) {
                addresses.push_back(entry.address);
                entry.valid = false;
            }
        }
        return addresses;
    }
    
    /**
     * @brief Get number of valid entries
     */
    size_t getOccupancy() const {
        return std::count_if(entries_.begin(), entries_.end(),
            [](const Entry& e) { return e.valid; });
    }
    
private:
    std::vector<Entry> entries_;
    uint64_t currentTime_;
};

/**
 * @class WritePolicyFactory
 * @brief Factory for creating write policy instances
 */
class WritePolicyFactory {
public:
    static std::unique_ptr<WritePolicyBase> create(
        WriteUpdatePolicy updatePolicy, 
        WriteAllocationPolicy allocPolicy) {
        
        if (updatePolicy == WriteUpdatePolicy::WriteThrough) {
            if (allocPolicy == WriteAllocationPolicy::WriteAllocate) {
                return std::make_unique<WriteThroughPolicy>();
            } else {
                return std::make_unique<WriteThroughNoAllocatePolicy>();
            }
        } else {
            if (allocPolicy == WriteAllocationPolicy::WriteAllocate) {
                return std::make_unique<WriteBackPolicy>();
            } else {
                return std::make_unique<WriteBackNoAllocatePolicy>();
            }
        }
    }
};

} // namespace cachesim
