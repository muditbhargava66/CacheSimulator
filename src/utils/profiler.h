#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <memory>
#include <optional>
#include <functional>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace cachesim {

/**
 * Memory access profiler for analyzing and categorizing memory access patterns.
 * Tracks different aspects of memory accesses to help understand program behavior.
 */
class MemoryProfiler {
public:
    // Structure to represent a memory region
    struct MemoryRegion {
        uint32_t startAddress;
        uint32_t endAddress;
        std::string name;
        
        bool contains(uint32_t address) const {
            return address >= startAddress && address <= endAddress;
        }
    };
    
    // Structure to track access statistics for a region
    struct RegionStats {
        size_t readCount = 0;
        size_t writeCount = 0;
        size_t uniqueAddresses = 0;
        uint32_t firstAccess = 0;
        uint32_t lastAccess = 0;
        std::chrono::steady_clock::time_point firstAccessTime;
        std::chrono::steady_clock::time_point lastAccessTime;
        std::unordered_map<uint32_t, size_t> addressFrequency;
        
        void addAccess(uint32_t address, bool isWrite) {
            if (isWrite) {
                writeCount++;
            } else {
                readCount++;
            }
            
            // Update address frequency
            if (addressFrequency.find(address) == addressFrequency.end()) {
                uniqueAddresses++;
            }
            addressFrequency[address]++;
            
            // Update first/last access
            if (firstAccess == 0 || address < firstAccess) {
                firstAccess = address;
                firstAccessTime = std::chrono::steady_clock::now();
            }
            
            lastAccess = address;
            lastAccessTime = std::chrono::steady_clock::now();
        }
        
        // Get total access count
        [[nodiscard]] size_t getTotalAccesses() const {
            return readCount + writeCount;
        }
        
        // Calculate read/write ratio
        [[nodiscard]] double getReadWriteRatio() const {
            return writeCount > 0 ? static_cast<double>(readCount) / writeCount : 0.0;
        }
        
        // Calculate duration in milliseconds
        [[nodiscard]] double getDurationMs() const {
            return std::chrono::duration<double, std::milli>(
                lastAccessTime - firstAccessTime).count();
        }
        
        // Find most frequently accessed address
        [[nodiscard]] std::optional<std::pair<uint32_t, size_t>> getMostFrequentAddress() const {
            if (addressFrequency.empty()) {
                return std::nullopt;
            }
            
            return *std::max_element(
                addressFrequency.begin(), addressFrequency.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            );
        }
    };
    
    // Memory access pattern types
    enum class PatternType {
        Unknown,
        Sequential,
        Strided,
        Random,
        Clustered,
        LoopingAccess
    };
    
    // Constructor
    MemoryProfiler() = default;
    
    // Define a memory region to track
    void defineRegion(uint32_t startAddress, uint32_t endAddress, std::string_view name) {
        regions.push_back({startAddress, endAddress, std::string(name)});
        regionStats[std::string(name)] = RegionStats{};
    }
    
    // Track a memory access
    void trackAccess(uint32_t address, bool isWrite) {
        // Track overall access
        accessCount++;
        if (isWrite) {
            writeCount++;
        } else {
            readCount++;
        }
        
        // Track last N addresses for pattern detection
        recentAddresses.push_back(address);
        if (recentAddresses.size() > maxRecentAddresses) {
            recentAddresses.erase(recentAddresses.begin());
        }
        
        // Update address frequency
        addressFrequency[address]++;
        
        // Find which region this belongs to
        bool foundRegion = false;
        for (const auto& region : regions) {
            if (region.contains(address)) {
                regionStats[region.name].addAccess(address, isWrite);
                foundRegion = true;
                break;
            }
        }
        
        // Track in "unknown" region if not matched
        if (!foundRegion) {
            regionStats["unknown"].addAccess(address, isWrite);
        }
        
        // Update strides
        if (lastAddress) {
            int32_t stride = static_cast<int32_t>(address) - static_cast<int32_t>(*lastAddress);
            strides.push_back(stride);
            strideFrequency[stride]++;
        }
        
        lastAddress = address;
    }
    
    // Detect memory access pattern
    [[nodiscard]] PatternType detectPattern() const {
        if (recentAddresses.size() < 10) {
            return PatternType::Unknown; // Not enough data
        }
        
        // Check for sequential access
        bool isSequential = true;
        for (size_t i = 1; i < recentAddresses.size(); ++i) {
            if (recentAddresses[i] != recentAddresses[i-1] + 1 &&
                recentAddresses[i] != recentAddresses[i-1] + 4 &&  // Word stride
                recentAddresses[i] != recentAddresses[i-1] + 8) {  // Double word stride
                isSequential = false;
                break;
            }
        }
        
        if (isSequential) {
            return PatternType::Sequential;
        }
        
        // Check for strided access
        if (!strides.empty()) {
            // Find most common stride
            auto mostCommonStride = std::max_element(
                strideFrequency.begin(), strideFrequency.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            );
            
            // Calculate stride consistency ratio
            double strideConsistency = static_cast<double>(mostCommonStride->second) / strides.size();
            
            if (strideConsistency > 0.7) {
                return PatternType::Strided;
            }
        }
        
        // Check for looping access (repeating sequence)
        for (size_t loopSize = 3; loopSize <= recentAddresses.size() / 3; ++loopSize) {
            bool isLoop = true;
            
            for (size_t i = loopSize; i < recentAddresses.size(); ++i) {
                if (recentAddresses[i] != recentAddresses[i % loopSize]) {
                    isLoop = false;
                    break;
                }
            }
            
            if (isLoop) {
                return PatternType::LoopingAccess;
            }
        }
        
        // Check for clustered access
        size_t uniqueAddresses = addressFrequency.size();
        double accessDensity = static_cast<double>(accessCount) / uniqueAddresses;
        
        if (accessDensity > 3.0) {
            return PatternType::Clustered;
        }
        
        // Default to random if no pattern is detected
        return PatternType::Random;
    }
    
    // Get memory region statistics
    [[nodiscard]] const RegionStats& getRegionStats(std::string_view name) const {
        static const RegionStats emptyStats;
        
        auto it = regionStats.find(std::string(name));
        if (it != regionStats.end()) {
            return it->second;
        }
        
        return emptyStats;
    }
    
    // Get all region statistics
    [[nodiscard]] const auto& getAllRegionStats() const {
        return regionStats;
    }
    
    // Get overall statistics
    [[nodiscard]] size_t getTotalAccesses() const {
        return accessCount;
    }
    
    [[nodiscard]] size_t getReadCount() const {
        return readCount;
    }
    
    [[nodiscard]] size_t getWriteCount() const {
        return writeCount;
    }
    
    [[nodiscard]] size_t getUniqueAddresses() const {
        return addressFrequency.size();
    }
    
    [[nodiscard]] double getReadWriteRatio() const {
        return writeCount > 0 ? static_cast<double>(readCount) / writeCount : 0.0;
    }
    
    // Get most frequently accessed address
    [[nodiscard]] std::optional<std::pair<uint32_t, size_t>> getMostFrequentAddress() const {
        if (addressFrequency.empty()) {
            return std::nullopt;
        }
        
        return *std::max_element(
            addressFrequency.begin(), addressFrequency.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; }
        );
    }
    
    // Get most common stride
    [[nodiscard]] std::optional<std::pair<int32_t, size_t>> getMostCommonStride() const {
        if (strideFrequency.empty()) {
            return std::nullopt;
        }
        
        return *std::max_element(
            strideFrequency.begin(), strideFrequency.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; }
        );
    }
    
    // Get memory footprint (total size of unique addresses accessed)
    [[nodiscard]] size_t getMemoryFootprint() const {
        if (addressFrequency.empty()) {
            return 0;
        }
        
        uint32_t minAddr = std::numeric_limits<uint32_t>::max();
        uint32_t maxAddr = 0;
        
        for (const auto& [addr, _] : addressFrequency) {
            minAddr = std::min(minAddr, addr);
            maxAddr = std::max(maxAddr, addr);
        }
        
        return maxAddr - minAddr + 1;
    }
    
    // Print profiling results
    void printResults(std::ostream& out = std::cout) const {
        out << "Memory Access Profiling Results" << std::endl;
        out << "===============================" << std::endl;
        
        // Print overall statistics
        out << "Total Accesses: " << accessCount << std::endl;
        out << "Read Accesses: " << readCount 
            << " (" << std::fixed << std::setprecision(1) 
            << (100.0 * readCount / accessCount) << "%)" << std::endl;
        out << "Write Accesses: " << writeCount 
            << " (" << std::fixed << std::setprecision(1) 
            << (100.0 * writeCount / accessCount) << "%)" << std::endl;
        out << "Unique Addresses: " << addressFrequency.size() << std::endl;
        out << "Memory Footprint: " << getMemoryFootprint() << " bytes" << std::endl;
        out << "Read/Write Ratio: " << std::fixed << std::setprecision(2) 
            << getReadWriteRatio() << std::endl;
        
        // Print detected pattern
        out << "Detected Access Pattern: ";
        switch (detectPattern()) {
            case PatternType::Sequential:
                out << "Sequential";
                break;
            case PatternType::Strided:
                out << "Strided";
                if (auto stride = getMostCommonStride()) {
                    out << " (common stride: " << stride->first << ", "
                        << std::fixed << std::setprecision(1)
                        << (100.0 * stride->second / strides.size()) << "% of accesses)";
                }
                break;
            case PatternType::Random:
                out << "Random";
                break;
            case PatternType::Clustered:
                out << "Clustered";
                break;
            case PatternType::LoopingAccess:
                out << "Looping";
                break;
            default:
                out << "Unknown";
                break;
        }
        out << std::endl << std::endl;
        
        // Print region statistics
        out << "Region Statistics:" << std::endl;
        out << "-----------------" << std::endl;
        
        for (const auto& region : regions) {
            const auto& stats = regionStats.at(region.name);
            
            out << "Region: " << region.name << std::endl;
            out << "  Address Range: 0x" << std::hex << region.startAddress 
                << " - 0x" << region.endAddress << std::dec << std::endl;
            out << "  Total Accesses: " << stats.getTotalAccesses() 
                << " (" << std::fixed << std::setprecision(1) 
                << (100.0 * stats.getTotalAccesses() / accessCount) << "%)" << std::endl;
            out << "  Read Accesses: " << stats.readCount << std::endl;
            out << "  Write Accesses: " << stats.writeCount << std::endl;
            out << "  Unique Addresses: " << stats.uniqueAddresses << std::endl;
            out << "  Read/Write Ratio: " << std::fixed << std::setprecision(2) 
                << stats.getReadWriteRatio() << std::endl;
            
            if (auto mostFrequent = stats.getMostFrequentAddress()) {
                out << "  Most Frequently Accessed: 0x" << std::hex << mostFrequent->first 
                    << std::dec << " (" << mostFrequent->second << " times)" << std::endl;
            }
            
            out << std::endl;
        }
        
        // Print "unknown" region if there were accesses
        auto unknownIt = regionStats.find("unknown");
        if (unknownIt != regionStats.end() && unknownIt->second.getTotalAccesses() > 0) {
            const auto& stats = unknownIt->second;
            
            out << "Region: unknown (addresses not in any defined region)" << std::endl;
            out << "  Total Accesses: " << stats.getTotalAccesses() 
                << " (" << std::fixed << std::setprecision(1) 
                << (100.0 * stats.getTotalAccesses() / accessCount) << "%)" << std::endl;
            out << "  Read Accesses: " << stats.readCount << std::endl;
            out << "  Write Accesses: " << stats.writeCount << std::endl;
            out << "  Unique Addresses: " << stats.uniqueAddresses << std::endl;
            
            if (auto mostFrequent = stats.getMostFrequentAddress()) {
                out << "  Most Frequently Accessed: 0x" << std::hex << mostFrequent->first 
                    << std::dec << " (" << mostFrequent->second << " times)" << std::endl;
            }
            
            out << std::endl;
        }
    }
    
    // Export profiling data to a file
    bool exportToFile(const std::filesystem::path& filePath) const {
        std::ofstream file(filePath);
        if (!file) {
            return false;
        }
        
        // Export overall statistics as CSV
        file << "Statistic,Value" << std::endl;
        file << "Total Accesses," << accessCount << std::endl;
        file << "Read Accesses," << readCount << std::endl;
        file << "Write Accesses," << writeCount << std::endl;
        file << "Unique Addresses," << addressFrequency.size() << std::endl;
        file << "Memory Footprint," << getMemoryFootprint() << std::endl;
        file << "Read/Write Ratio," << getReadWriteRatio() << std::endl;
        
        // Export detected pattern
        file << "Detected Pattern,";
        switch (detectPattern()) {
            case PatternType::Sequential: file << "Sequential"; break;
            case PatternType::Strided: file << "Strided"; break;
            case PatternType::Random: file << "Random"; break;
            case PatternType::Clustered: file << "Clustered"; break;
            case PatternType::LoopingAccess: file << "Looping"; break;
            default: file << "Unknown"; break;
        }
        file << std::endl << std::endl;
        
        // Export region statistics
        file << "Region Statistics" << std::endl;
        file << "Region,Start Address,End Address,Total Accesses,Read Accesses,Write Accesses,"
             << "Unique Addresses,Read/Write Ratio" << std::endl;
        
        for (const auto& region : regions) {
            const auto& stats = regionStats.at(region.name);
            
            file << region.name << ","
                 << "0x" << std::hex << region.startAddress << ","
                 << "0x" << region.endAddress << std::dec << ","
                 << stats.getTotalAccesses() << ","
                 << stats.readCount << ","
                 << stats.writeCount << ","
                 << stats.uniqueAddresses << ","
                 << stats.getReadWriteRatio() << std::endl;
        }
        
        // Export address frequency data
        file << std::endl << "Address Frequency" << std::endl;
        file << "Address,Frequency" << std::endl;
        
        for (const auto& [addr, freq] : addressFrequency) {
            if (freq > 1) { // Only export addresses accessed more than once
                file << "0x" << std::hex << addr << std::dec << "," << freq << std::endl;
            }
        }
        
        // Export stride frequency data
        file << std::endl << "Stride Frequency" << std::endl;
        file << "Stride,Frequency,Percentage" << std::endl;
        
        for (const auto& [stride, freq] : strideFrequency) {
            if (freq > 1) { // Only export strides that occurred more than once
                file << stride << "," << freq << "," 
                     << (100.0 * freq / strides.size()) << std::endl;
            }
        }
        
        return true;
    }
    
    // Reset profiler
    void reset() {
        accessCount = 0;
        readCount = 0;
        writeCount = 0;
        lastAddress.reset();
        recentAddresses.clear();
        addressFrequency.clear();
        strides.clear();
        strideFrequency.clear();
        
        // Reset region stats
        for (auto& [name, stats] : regionStats) {
            stats = RegionStats{};
        }
    }

private:
    size_t accessCount = 0;
    size_t readCount = 0;
    size_t writeCount = 0;
    std::optional<uint32_t> lastAddress;
    std::vector<uint32_t> recentAddresses;
    std::unordered_map<uint32_t, size_t> addressFrequency;
    std::vector<int32_t> strides;
    std::unordered_map<int32_t, size_t> strideFrequency;
    
    std::vector<MemoryRegion> regions;
    std::unordered_map<std::string, RegionStats> regionStats;
    
    static constexpr size_t maxRecentAddresses = 100; // Keep track of last 100 addresses
};

} // namespace cachesim