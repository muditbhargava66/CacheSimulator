/**
 * @file processor_core.cpp
 * @brief Implementation of processor core for multi-processor simulation
 * @author Mudit Bhargava
 * @date 2025-05-31
 * @version 1.2.0
 */

#include "processor_core.h"
#include "coherence_controller.h"
#include "../../utils/trace_parser.h"
#include <iostream>
#include <iomanip>
#include <thread>

namespace cachesim {

ProcessorCore::ProcessorCore(uint32_t id, const CacheConfig& l1Config,
                           std::shared_ptr<CoherenceController> coherenceController)
    : processorId_(id),
      state_(ProcessorState::Idle),
      l1Cache_(l1Config),
      coherenceController_(coherenceController) {
}

uint32_t ProcessorCore::access(uint32_t address, bool isWrite) {
    totalAccesses_++;
    uint32_t latency = 1; // Base latency
    
    // Check coherence permission first
    if (!checkCoherencePermission(address, isWrite)) {
        state_ = ProcessorState::Stalled;
        
        // Send coherence request
        CoherenceRequest request{
            .requestorId = processorId_,
            .address = address,
            .type = isWrite ? CoherenceRequestType::WriteRequest : CoherenceRequestType::ReadRequest,
            .currentState = l1Cache_.getBlockState(address),
            .needsData = true,
            .timestamp = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())
        };
        
        auto response = coherenceController_->processRequest(request);
        latency += response.latency;
        
        if (!response.granted) {
            coherenceMisses_++;
            latency += handleL1Miss(address, isWrite);
        }
        
        state_ = ProcessorState::Running;
    }
    
    // Access L1 cache
    bool hit = l1Cache_.access(address, isWrite);
    
    if (hit) {
        l1Hits_++;
        latency += 1; // L1 hit latency
    } else {
        l1Misses_++;
        latency += handleL1Miss(address, isWrite);
    }
    
    totalLatency_ += latency;
    return latency;
}

uint32_t ProcessorCore::atomicAccess(uint32_t address, bool isWrite) {
    atomicOperations_++;
    
    // Atomic operations require exclusive access
    state_ = ProcessorState::Synchronizing;
    
    // Get exclusive ownership
    CoherenceRequest request{
        .requestorId = processorId_,
        .address = address,
        .type = CoherenceRequestType::WriteRequest,
        .currentState = l1Cache_.getBlockState(address),
        .needsData = true,
        .timestamp = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())
    };
    
    auto response = coherenceController_->processRequest(request);
    uint32_t latency = response.latency;
    
    // Perform the access
    latency += access(address, isWrite);
    
    // Ensure atomicity by holding exclusive state
    if (isWrite) {
        updateCoherenceState(address, MESIState::Modified);
    }
    
    state_ = ProcessorState::Running;
    return latency;
}

uint32_t ProcessorCore::handleCoherenceRequest(const CoherenceRequest& request) {
    uint32_t latency = 1;
    
    // Get current state of the block
    auto currentState = l1Cache_.getBlockState(request.address);
    
    switch (request.type) {
        case CoherenceRequestType::InvalidateRequest:
            if (currentState != MESIState::Invalid) {
                l1Cache_.invalidateBlock(request.address);
                coherenceController_->updateCacheState(
                    processorId_, request.address, currentState, MESIState::Invalid);
                latency += 2; // Invalidation latency
            }
            break;
            
        case CoherenceRequestType::ShareRequest:
            if (currentState == MESIState::Modified || currentState == MESIState::Exclusive) {
                // Downgrade to shared
                l1Cache_.updateMESIState(request.address, MESIState::Shared);
                coherenceController_->updateCacheState(
                    processorId_, request.address, currentState, MESIState::Shared);
                
                if (currentState == MESIState::Modified) {
                    // Need to write back data
                    latency += 10; // Writeback latency
                }
            }
            break;
            
        case CoherenceRequestType::FlushRequest:
            if (currentState == MESIState::Modified) {
                // Write back and invalidate
                latency += 10; // Writeback latency
            }
            l1Cache_.invalidateBlock(request.address);
            coherenceController_->updateCacheState(
                processorId_, request.address, currentState, MESIState::Invalid);
            break;
            
        default:
            break;
    }
    
    return latency;
}

void ProcessorCore::memoryBarrier(bool isAcquire, bool isRelease) {
    barriers_++;
    
    std::lock_guard<std::mutex> lock(orderingMutex_);
    
    if (isRelease) {
        // Ensure all previous writes are visible
        drainPendingRequests();
        pendingReleases_.clear();
    }
    
    if (isAcquire) {
        // Ensure no future reads see stale data
        // This is handled by coherence protocol
    }
}

ProcessorCore::ProcessorStats ProcessorCore::getStats() const {
    ProcessorStats stats;
    stats.totalAccesses = totalAccesses_;
    stats.l1Hits = l1Hits_;
    stats.l1Misses = l1Misses_;
    stats.coherenceMisses = coherenceMisses_;
    stats.atomicOperations = atomicOperations_;
    stats.barriers = barriers_;
    stats.stalledCycles = stalledCycles_;
    stats.avgLatency = totalAccesses_ > 0 ? 
        static_cast<double>(totalLatency_) / totalAccesses_ : 0.0;
    return stats;
}

void ProcessorCore::resetStats() {
    totalAccesses_ = 0;
    l1Hits_ = 0;
    l1Misses_ = 0;
    coherenceMisses_ = 0;
    atomicOperations_ = 0;
    barriers_ = 0;
    stalledCycles_ = 0;
    totalLatency_ = 0;
    l1Cache_.resetStats();
}

void ProcessorCore::printStats() const {
    auto stats = getStats();
    std::cout << "Processor " << processorId_ << " Statistics:" << std::endl;
    std::cout << "  Total Accesses: " << stats.totalAccesses << std::endl;
    std::cout << "  L1 Hits: " << stats.l1Hits 
              << " (" << std::fixed << std::setprecision(2)
              << (stats.l1Hits * 100.0 / stats.totalAccesses) << "%)" << std::endl;
    std::cout << "  L1 Misses: " << stats.l1Misses << std::endl;
    std::cout << "  Coherence Misses: " << stats.coherenceMisses << std::endl;
    std::cout << "  Atomic Operations: " << stats.atomicOperations << std::endl;
    std::cout << "  Memory Barriers: " << stats.barriers << std::endl;
    std::cout << "  Average Latency: " << std::fixed << std::setprecision(2)
              << stats.avgLatency << " cycles" << std::endl;
    std::cout << std::endl;
}

uint32_t ProcessorCore::handleL1Miss(uint32_t address, bool isWrite) {
    uint32_t latency = 10; // Base miss penalty
    
    // Update coherence state
    MESIState newState = isWrite ? MESIState::Modified : MESIState::Shared;
    coherenceController_->updateCacheState(processorId_, address, 
                                          MESIState::Invalid, newState);
    
    return latency;
}

void ProcessorCore::updateCoherenceState(uint32_t address, MESIState newState) {
    auto oldState = l1Cache_.getBlockState(address);
    l1Cache_.updateMESIState(address, newState);
    coherenceController_->updateCacheState(processorId_, address, oldState, newState);
}

bool ProcessorCore::checkCoherencePermission(uint32_t address, bool isWrite) {
    return coherenceController_->checkPermission(processorId_, address, isWrite);
}

void ProcessorCore::drainPendingRequests() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    while (!requestQueue_.empty()) {
        queueCV_.wait_for(lock, std::chrono::milliseconds(1));
    }
}

// MultiProcessorSystem Implementation

MultiProcessorSystem::MultiProcessorSystem(const Config& config)
    : config_(config) {
    initializeSystem();
}

void MultiProcessorSystem::initializeSystem() {
    // Create coherence controller
    coherenceController_ = std::make_shared<CoherenceController>(
        config_.numProcessors, config_.interconnectLatency);
    
    // Create processors
    processors_.reserve(config_.numProcessors);
    std::vector<ProcessorCore*> processorPtrs;
    
    for (uint32_t i = 0; i < config_.numProcessors; ++i) {
        processors_.emplace_back(std::make_unique<ProcessorCore>(
            i, config_.l1Config, coherenceController_));
        processorPtrs.push_back(processors_.back().get());
    }
    
    // Set processor references in coherence controller
    coherenceController_->setProcessors(processorPtrs);
    
    // Create shared caches if configured
    if (config_.sharedL2Config) {
        sharedL2_ = std::make_unique<Cache>(*config_.sharedL2Config);
    }
    
    if (config_.sharedL3Config) {
        sharedL3_ = std::make_unique<Cache>(*config_.sharedL3Config);
    }
}

ProcessorCore* MultiProcessorSystem::getProcessor(uint32_t id) {
    if (id < processors_.size()) {
        return processors_[id].get();
    }
    return nullptr;
}

const ProcessorCore* MultiProcessorSystem::getProcessor(uint32_t id) const {
    if (id < processors_.size()) {
        return processors_[id].get();
    }
    return nullptr;
}

uint32_t MultiProcessorSystem::processAccess(uint32_t processorId, 
                                            uint32_t address, bool isWrite) {
    auto* processor = getProcessor(processorId);
    if (!processor) {
        throw std::runtime_error("Invalid processor ID");
    }
    
    uint32_t latency = processor->access(address, isWrite);
    systemCycles_ += latency;
    return latency;
}

uint32_t MultiProcessorSystem::processAtomicAccess(uint32_t processorId,
                                                  uint32_t address, bool isWrite) {
    auto* processor = getProcessor(processorId);
    if (!processor) {
        throw std::runtime_error("Invalid processor ID");
    }
    
    return processor->atomicAccess(address, isWrite);
}

void MultiProcessorSystem::executeBarrier(uint32_t processorId,
                                        bool isAcquire, bool isRelease) {
    auto* processor = getProcessor(processorId);
    if (!processor) {
        throw std::runtime_error("Invalid processor ID");
    }
    
    processor->memoryBarrier(isAcquire, isRelease);
}

void MultiProcessorSystem::globalBarrier() {
    std::unique_lock<std::mutex> lock(barrierMutex_);
    
    uint32_t count = ++barrierCount_;
    if (count < config_.numProcessors) {
        // Wait for all processors
        barrierCV_.wait(lock, [this] {
            return barrierCount_ >= config_.numProcessors;
        });
    } else {
        // Last processor, wake all others
        barrierCount_ = 0;
        barrierCV_.notify_all();
    }
}

MultiProcessorSystem::SystemStats MultiProcessorSystem::getSystemStats() const {
    SystemStats stats{};
    
    for (const auto& processor : processors_) {
        auto pStats = processor->getStats();
        stats.totalAccesses += pStats.totalAccesses;
        stats.totalHits += pStats.l1Hits;
        stats.totalMisses += pStats.l1Misses;
    }
    
    auto coherenceStats = coherenceController_->getStats();
    stats.coherenceTraffic = coherenceStats.coherenceMessages;
    stats.interconnectMessages = coherenceStats.coherenceMessages;
    
    stats.avgLatency = stats.totalAccesses > 0 ?
        static_cast<double>(systemCycles_) / stats.totalAccesses : 0.0;
    
    stats.systemThroughput = systemCycles_ > 0 ?
        static_cast<double>(stats.totalAccesses) / systemCycles_ : 0.0;
    
    return stats;
}

void MultiProcessorSystem::printSystemStats() const {
    std::cout << "Multi-Processor System Statistics" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Print per-processor stats
    for (uint32_t i = 0; i < config_.numProcessors; ++i) {
        processors_[i]->printStats();
    }
    
    // Print system-wide stats
    auto stats = getSystemStats();
    std::cout << "System-wide Statistics:" << std::endl;
    std::cout << "  Total Accesses: " << stats.totalAccesses << std::endl;
    std::cout << "  Total Hits: " << stats.totalHits << std::endl;
    std::cout << "  Total Misses: " << stats.totalMisses << std::endl;
    std::cout << "  System Hit Rate: " << std::fixed << std::setprecision(2)
              << (stats.totalHits * 100.0 / stats.totalAccesses) << "%" << std::endl;
    std::cout << "  Coherence Traffic: " << stats.coherenceTraffic << " messages" << std::endl;
    std::cout << "  Average Latency: " << stats.avgLatency << " cycles" << std::endl;
    std::cout << "  System Throughput: " << std::setprecision(4)
              << stats.systemThroughput << " accesses/cycle" << std::endl;
    
    // Print coherence controller stats
    std::cout << std::endl;
    coherenceController_->printStats();
}

void MultiProcessorSystem::resetAllStats() {
    for (auto& processor : processors_) {
        processor->resetStats();
    }
    coherenceController_->resetStats();
    systemCycles_ = 0;
    coherenceMessages_ = 0;
}

uint64_t MultiProcessorSystem::simulateParallelTraces(
    const std::vector<std::string>& traces) {
    
    if (traces.size() != config_.numProcessors) {
        throw std::runtime_error("Number of traces must match number of processors");
    }
    
    std::vector<std::thread> threads;
    std::atomic<uint64_t> maxCycles{0};
    
    // Launch thread for each processor
    for (uint32_t i = 0; i < config_.numProcessors; ++i) {
        threads.emplace_back([this, i, &traces, &maxCycles]() {
            TraceParser parser(traces[i]);
            uint64_t localCycles = 0;
            
            while (auto access = parser.getNextAccess()) {
                uint32_t latency = processAccess(i, access->address, access->isWrite);
                localCycles += latency;
            }
            
            // Update max cycles
            uint64_t current = maxCycles.load();
            while (localCycles > current && 
                   !maxCycles.compare_exchange_weak(current, localCycles));
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    return maxCycles.load();
}

} // namespace cachesim
