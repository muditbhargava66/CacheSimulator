/**
 * @file coherence_controller.cpp
 * @brief Implementation of cache coherence controller
 * @author Mudit Bhargava
 * @date 2025-05-31
 * @version 1.2.0
 */

#include "coherence_controller.h"
#include "processor_core.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace cachesim {

CoherenceController::CoherenceController(uint32_t numProcessors, uint32_t interconnectLatency)
    : numProcessors_(numProcessors), interconnectLatency_(interconnectLatency) {
}

CoherenceResponse CoherenceController::processRequest(const CoherenceRequest& request) {
    std::lock_guard<std::mutex> lock(directoryMutex_);
    
    CoherenceResponse response;
    response.granted = false;
    response.hasData = false;
    response.latency = interconnectLatency_;
    
    // Get or create directory entry
    DirectoryEntry& entry = getOrCreateEntry(request.address);
    
    switch (request.type) {
        case CoherenceRequestType::ReadRequest:
            readRequests_++;
            if (entry.state_ == MESIState::Invalid) {
                // No one has it - grant exclusive
                entry.state_ = MESIState::Exclusive;
                entry.owner_ = request.requestorId;
                response.granted = true;
                response.newState = MESIState::Exclusive;
            } else if (entry.state_ == MESIState::Exclusive || entry.state_ == MESIState::Modified) {
                // Someone has it exclusively
                if (entry.owner_ == static_cast<int32_t>(request.requestorId)) {
                    // Requestor already has it
                    response.granted = true;
                    response.newState = entry.state_;
                } else {
                    // Need to downgrade owner to shared
                    entry.state_ = MESIState::Shared;
                    entry.sharers_.insert(entry.owner_);
                    entry.sharers_.insert(request.requestorId);
                    entry.owner_ = -1;
                    response.granted = true;
                    response.newState = MESIState::Shared;
                    response.latency += interconnectLatency_; // Extra hop
                }
            } else if (entry.state_ == MESIState::Shared) {
                // Already shared - add to sharers
                entry.sharers_.insert(request.requestorId);
                response.granted = true;
                response.newState = MESIState::Shared;
            }
            break;
            
        case CoherenceRequestType::WriteRequest:
            writeRequests_++;
            if (entry.state_ == MESIState::Invalid) {
                // No one has it - grant modified
                entry.state_ = MESIState::Modified;
                entry.owner_ = request.requestorId;
                entry.dirty_ = true;
                response.granted = true;
                response.newState = MESIState::Modified;
            } else if (entry.state_ == MESIState::Exclusive || entry.state_ == MESIState::Modified) {
                if (entry.owner_ == static_cast<int32_t>(request.requestorId)) {
                    // Upgrade to modified
                    entry.state_ = MESIState::Modified;
                    entry.dirty_ = true;
                    response.granted = true;
                    response.newState = MESIState::Modified;
                } else {
                    // Need to invalidate owner
                    response.invalidatedProcessors.push_back(entry.owner_);
                    entry.state_ = MESIState::Modified;
                    entry.owner_ = request.requestorId;
                    entry.dirty_ = true;
                    response.granted = true;
                    response.newState = MESIState::Modified;
                    response.latency += interconnectLatency_;
                    invalidations_++;
                }
            } else if (entry.state_ == MESIState::Shared) {
                // Need to invalidate all sharers
                response.invalidatedProcessors = invalidateSharers(request.address, request.requestorId);
                entry.state_ = MESIState::Modified;
                entry.owner_ = request.requestorId;
                entry.sharers_.clear();
                entry.dirty_ = true;
                response.granted = true;
                response.newState = MESIState::Modified;
                response.latency += interconnectLatency_ * response.invalidatedProcessors.size();
                invalidations_ += response.invalidatedProcessors.size();
            }
            break;
            
        case CoherenceRequestType::InvalidateRequest:
            invalidations_++;
            response.invalidatedProcessors = invalidateSharers(request.address, request.requestorId);
            entry.state_ = MESIState::Invalid;
            entry.owner_ = -1;
            entry.sharers_.clear();
            response.granted = true;
            break;
            
        case CoherenceRequestType::WritebackRequest:
            writebacks_++;
            if (entry.owner_ == static_cast<int32_t>(request.requestorId)) {
                entry.dirty_ = false;
                response.granted = true;
            }
            break;
    }
    
    coherenceMessages_++;
    entry.lastAccess_ = std::chrono::steady_clock::now().time_since_epoch().count();
    
    return response;
}

void CoherenceController::updateCacheState(uint32_t processorId, uint32_t address,
                                         MESIState oldState, MESIState newState) {
    std::lock_guard<std::mutex> lock(directoryMutex_);
    
    stateTransitions_++;
    
    // Update directory
    DirectoryEntry& entry = getOrCreateEntry(address);
    
    if (newState == MESIState::Invalid) {
        entry.sharers_.erase(processorId);
        if (entry.owner_ == static_cast<int32_t>(processorId)) {
            entry.owner_ = -1;
        }
    }
}

void CoherenceController::handleWriteback(uint32_t processorId, uint32_t address,
                                        const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(directoryMutex_);
    
    writebacks_++;
    
    DirectoryEntry& entry = getOrCreateEntry(address);
    if (entry.owner_ == static_cast<int32_t>(processorId)) {
        entry.dirty_ = false;
        // In a real system, would write data to memory
    }
}

bool CoherenceController::checkPermission(uint32_t processorId, uint32_t address, bool isWrite) {
    std::lock_guard<std::mutex> lock(directoryMutex_);
    
    auto it = directory_.find(address);
    if (it == directory_.end()) {
        return true; // No entry means no restrictions
    }
    
    const DirectoryEntry& entry = it->second;
    
    if (isWrite) {
        // Write needs exclusive access
        return entry.state_ == MESIState::Modified && entry.owner_ == static_cast<int32_t>(processorId);
    } else {
        // Read allowed in several states
        if (entry.state_ == MESIState::Shared) {
            return entry.sharers_.count(processorId) > 0;
        } else if (entry.state_ == MESIState::Exclusive || entry.state_ == MESIState::Modified) {
            return entry.owner_ == static_cast<int32_t>(processorId);
        }
    }
    
    return false;
}

MESIState CoherenceController::getCacheLineState(uint32_t processorId, uint32_t address) const {
    std::lock_guard<std::mutex> lock(directoryMutex_);
    
    auto it = directory_.find(address);
    if (it == directory_.end()) {
        return MESIState::Invalid;
    }
    
    const DirectoryEntry& entry = it->second;
    
    if (entry.state_ == MESIState::Shared && entry.sharers_.count(processorId) > 0) {
        return MESIState::Shared;
    } else if ((entry.state_ == MESIState::Exclusive || entry.state_ == MESIState::Modified) &&
               entry.owner_ == static_cast<int32_t>(processorId)) {
        return entry.state_;
    }
    
    return MESIState::Invalid;
}

CoherenceController::CoherenceStats CoherenceController::getStats() const {
    CoherenceStats stats;
    stats.readRequests = readRequests_;
    stats.writeRequests = writeRequests_;
    stats.invalidations = invalidations_;
    stats.writebacks = writebacks_;
    stats.stateTransitions = stateTransitions_;
    stats.coherenceMessages = coherenceMessages_;
    return stats;
}

void CoherenceController::resetStats() {
    readRequests_ = 0;
    writeRequests_ = 0;
    invalidations_ = 0;
    writebacks_ = 0;
    stateTransitions_ = 0;
    coherenceMessages_ = 0;
}

void CoherenceController::printStats() const {
    auto stats = getStats();
    std::cout << "Coherence Controller Statistics:" << std::endl;
    std::cout << "  Read Requests: " << stats.readRequests << std::endl;
    std::cout << "  Write Requests: " << stats.writeRequests << std::endl;
    std::cout << "  Invalidations: " << stats.invalidations << std::endl;
    std::cout << "  Writebacks: " << stats.writebacks << std::endl;
    std::cout << "  State Transitions: " << stats.stateTransitions << std::endl;
    std::cout << "  Coherence Messages: " << stats.coherenceMessages << std::endl;
    std::cout << "  Avg Messages per Request: " << std::fixed << std::setprecision(2)
              << (static_cast<double>(stats.coherenceMessages) / 
                  (stats.readRequests + stats.writeRequests)) << std::endl;
}

DirectoryEntry& CoherenceController::getOrCreateEntry(uint32_t address) {
    return directory_[address];
}

std::vector<uint32_t> CoherenceController::invalidateSharers(uint32_t address, uint32_t excludeProcessor) {
    std::vector<uint32_t> invalidated;
    
    auto it = directory_.find(address);
    if (it != directory_.end()) {
        DirectoryEntry& entry = it->second;
        
        // Collect all processors to invalidate
        for (uint32_t sharer : entry.sharers_) {
            if (sharer != excludeProcessor) {
                invalidated.push_back(sharer);
            }
        }
        
        if (entry.owner_ >= 0 && entry.owner_ != static_cast<int32_t>(excludeProcessor)) {
            invalidated.push_back(entry.owner_);
        }
    }
    
    return invalidated;
}

void CoherenceController::broadcastInvalidate(uint32_t address, const std::vector<uint32_t>& processors) {
    for (uint32_t processorId : processors) {
        if (processorId < processors_.size() && processors_[processorId]) {
            CoherenceRequest invRequest{
                .requestorId = numProcessors_, // System-initiated
                .address = address,
                .type = CoherenceRequestType::InvalidateRequest,
                .currentState = MESIState::Invalid,
                .needsData = false,
                .timestamp = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count())
            };
            
            processors_[processorId]->handleCoherenceRequest(invRequest);
        }
    }
}

std::string CoherenceController::getTransitionKey(MESIState from, MESIState to) const {
    std::stringstream ss;
    ss << static_cast<int>(from) << "->" << static_cast<int>(to);
    return ss.str();
}

} // namespace cachesim
