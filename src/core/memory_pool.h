/**
 * @file memory_pool.h
 * @brief Memory pool allocator for efficient cache block management
 * @author Mudit Bhargava
 * @date 2025-05-27
 * @version 1.1.0
 *
 * This file implements a memory pool allocator to reduce dynamic allocation
 * overhead when managing cache blocks. The pool pre-allocates memory in
 * chunks and provides fast allocation/deallocation operations.
 */

#pragma once

#include <memory>
#include <vector>
#include <cstddef>
#include <utility>
#include <type_traits>

namespace cachesim {

/**
 * @class MemoryPool
 * @brief Fixed-size memory pool for efficient object allocation
 *
 * This class implements a memory pool that pre-allocates memory in chunks
 * to avoid frequent dynamic allocations. Objects are allocated from the
 * pool and returned to it on deallocation. The pool automatically grows
 * when needed.
 *
 * @tparam T The type of objects to be allocated from the pool
 */
template<typename T>
class MemoryPool {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    /**
     * @brief Construct a memory pool with specified chunk size
     * @param poolSize Number of objects per chunk (default: 4096)
     */
    explicit MemoryPool(size_type poolSize = 4096) 
        : poolSize_(poolSize), currentBlock_(0), currentIndex_(0) {
        allocateNewBlock();
    }

    // Disable copy operations
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    /**
     * @brief Move constructor
     * @param other Pool to move from
     */
    MemoryPool(MemoryPool&& other) noexcept
        : blocks_(std::move(other.blocks_)),
          poolSize_(other.poolSize_),
          currentBlock_(other.currentBlock_),
          currentIndex_(other.currentIndex_) {
        other.currentBlock_ = 0;
        other.currentIndex_ = 0;
    }

    /**
     * @brief Move assignment operator
     * @param other Pool to move from
     * @return Reference to this pool
     */
    MemoryPool& operator=(MemoryPool&& other) noexcept {
        if (this != &other) {
            blocks_ = std::move(other.blocks_);
            poolSize_ = other.poolSize_;
            currentBlock_ = other.currentBlock_;
            currentIndex_ = other.currentIndex_;
            other.currentBlock_ = 0;
            other.currentIndex_ = 0;
        }
        return *this;
    }

    ~MemoryPool() = default;

    /**
     * @brief Allocate memory for n objects
     * @param n Number of objects to allocate (default: 1)
     * @return Pointer to allocated memory
     */
    [[nodiscard]] pointer allocate(size_type n = 1) {
        if (n > poolSize_ - currentIndex_) {
            allocateNewBlock();
        }

        pointer result = &blocks_[currentBlock_][currentIndex_];
        currentIndex_ += n;
        return result;
    }

    /**
     * @brief Deallocate memory (no-op for pool allocator)
     * @param p Pointer to deallocate
     * @param n Number of objects
     */
    void deallocate(pointer p, size_type n = 1) noexcept {
        // Pool allocator doesn't deallocate individual objects
        // Memory is reclaimed when the pool is destroyed
        (void)p;
        (void)n;
    }

    /**
     * @brief Construct object in allocated memory
     * @tparam U Object type
     * @tparam Args Constructor argument types
     * @param p Pointer to memory
     * @param args Constructor arguments
     */
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }

    /**
     * @brief Destroy object in allocated memory
     * @tparam U Object type
     * @param p Pointer to object
     */
    template<typename U>
    void destroy(U* p) noexcept {
        p->~U();
    }

    /**
     * @brief Reset the pool to initial state
     */
    void reset() noexcept {
        currentBlock_ = 0;
        currentIndex_ = 0;
    }

    /**
     * @brief Get total capacity of the pool
     * @return Total number of objects that can be allocated
     */
    [[nodiscard]] size_type capacity() const noexcept {
        return blocks_.size() * poolSize_;
    }

    /**
     * @brief Get current size of allocated objects
     * @return Number of objects currently allocated
     */
    [[nodiscard]] size_type size() const noexcept {
        return currentBlock_ * poolSize_ + currentIndex_;
    }

private:
    /**
     * @brief Allocate a new block of memory
     */
    void allocateNewBlock() {
        if (currentBlock_ >= blocks_.size()) {
            blocks_.emplace_back(std::make_unique<T[]>(poolSize_));
        }
        currentIndex_ = 0;
        ++currentBlock_;
    }

    std::vector<std::unique_ptr<T[]>> blocks_;  ///< Memory blocks
    size_type poolSize_;                        ///< Objects per block
    size_type currentBlock_;                    ///< Current block index
    size_type currentIndex_;                    ///< Current position in block
};

/**
 * @class PoolAllocator
 * @brief STL-compatible allocator adapter for MemoryPool
 *
 * This class adapts the MemoryPool to work with STL containers,
 * providing the standard allocator interface required by containers
 * like std::vector, std::list, etc.
 *
 * @tparam T The type of objects to be allocated
 */
template<typename T>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };

    /**
     * @brief Construct allocator with reference to memory pool
     * @param pool Reference to the memory pool to use
     */
    explicit PoolAllocator(MemoryPool<T>& pool) noexcept : pool_(&pool) {}

    /**
     * @brief Copy constructor for different types
     * @tparam U Other type
     * @param other Allocator to copy from
     */
    template<typename U>
    PoolAllocator(const PoolAllocator<U>& other) noexcept : pool_(other.pool_) {}

    /**
     * @brief Allocate memory for n objects
     * @param n Number of objects
     * @return Pointer to allocated memory
     */
    [[nodiscard]] pointer allocate(size_type n) {
        return pool_->allocate(n);
    }

    /**
     * @brief Deallocate memory
     * @param p Pointer to deallocate
     * @param n Number of objects
     */
    void deallocate(pointer p, size_type n) noexcept {
        pool_->deallocate(p, n);
    }

    /**
     * @brief Construct object in allocated memory
     * @tparam U Object type
     * @tparam Args Constructor argument types
     * @param p Pointer to memory
     * @param args Constructor arguments
     */
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        pool_->construct(p, std::forward<Args>(args)...);
    }

    /**
     * @brief Destroy object
     * @tparam U Object type
     * @param p Pointer to object
     */
    template<typename U>
    void destroy(U* p) noexcept {
        pool_->destroy(p);
    }

    /**
     * @brief Equality comparison
     * @param other Allocator to compare with
     * @return True if allocators use the same pool
     */
    bool operator==(const PoolAllocator& other) const noexcept {
        return pool_ == other.pool_;
    }

    /**
     * @brief Inequality comparison
     * @param other Allocator to compare with
     * @return True if allocators use different pools
     */
    bool operator!=(const PoolAllocator& other) const noexcept {
        return !(*this == other);
    }

private:
    MemoryPool<T>* pool_;  ///< Pointer to the memory pool

    template<typename U>
    friend class PoolAllocator;
};

} // namespace cachesim
