#include "trace_utils.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <random>
#include <set>
#include <unordered_set>
#include <map>

namespace cachesim {

// TraceAnalyzer implementation
TraceAnalyzer::TraceAnalyzer(const std::filesystem::path& filepath) 
    : parser(filepath) {
}

TraceStatistics TraceAnalyzer::analyzeTrace() {
    if (!parser.isValid()) {
        std::cerr << "Cannot analyze invalid trace file." << std::endl;
        return {};
    }
    
    TraceStatistics stats;
    std::vector<MemoryAccess> accesses = parser.parseAll();
    
    // Basic statistics
    stats.totalAccesses = accesses.size();
    stats.readAccesses = std::count_if(accesses.begin(), accesses.end(), 
                                      [](const auto& access) { return !access.isWrite; });
    stats.writeAccesses = stats.totalAccesses - stats.readAccesses;
    
    // Access frequency map
    for (const auto& access : accesses) {
        stats.accessFrequency[access.address]++;
    }
    
    // Detect pattern
    stats.detectedPattern = detectPattern();
    
    // Get hot ranges
    stats.hotRanges = getHotRanges();
    
    return stats;
}

std::optional<TraceStatistics::PatternInfo> TraceAnalyzer::detectPattern() {
    std::vector<MemoryAccess> accesses = parser.parseAll();
    if (accesses.size() < 10) {
        return std::nullopt; // Not enough data
    }
    
    // Check for sequential pattern
    TraceStatistics::PatternInfo pattern;
    double seqConfidence = 0.0;
    if (isSequentialPattern(accesses, seqConfidence)) {
        pattern.type = TraceStatistics::PatternType::Sequential;
        pattern.confidence = seqConfidence;
        pattern.stride = 1; // Sequential has stride of 1
        return pattern;
    }
    
    // Check for strided pattern
    double strideConfidence = 0.0;
    auto detectedStride = detectStride(accesses, strideConfidence);
    if (detectedStride) {
        pattern.type = TraceStatistics::PatternType::Strided;
        pattern.confidence = strideConfidence;
        pattern.stride = *detectedStride;
        return pattern;
    }
    
    // Check for looping pattern
    double loopConfidence = 0.0;
    size_t loopSize = 0;
    if (isLoopingPattern(accesses, loopSize, loopConfidence)) {
        pattern.type = TraceStatistics::PatternType::LoopingAccess;
        pattern.confidence = loopConfidence;
        pattern.loopSize = loopSize;
        return pattern;
    }
    
    // Default to random
    pattern.type = TraceStatistics::PatternType::Random;
    pattern.confidence = 0.5; // Moderate confidence
    return pattern;
}

bool TraceAnalyzer::isSequentialPattern(const std::vector<MemoryAccess>& accesses, double& confidence) const {
    if (accesses.size() < 10) {
        confidence = 0.0;
        return false;
    }
    
    int sequentialCount = 0;
    
    // Count sequential address transitions
    for (size_t i = 1; i < accesses.size(); i++) {
        if (accesses[i].address == accesses[i-1].address + 4 || // Word increment
            accesses[i].address == accesses[i-1].address + 8 || // Double word increment
            accesses[i].address == accesses[i-1].address + 1) { // Byte increment
            sequentialCount++;
        }
    }
    
    confidence = static_cast<double>(sequentialCount) / (accesses.size() - 1);
    return confidence > 0.7; // Consider it sequential if 70% of accesses follow the pattern
}

std::optional<int32_t> TraceAnalyzer::detectStride(const std::vector<MemoryAccess>& accesses, double& confidence) const {
    if (accesses.size() < 10) {
        confidence = 0.0;
        return std::nullopt;
    }
    
    // Count differences between addresses
    std::map<int32_t, int> strideCounts;
    for (size_t i = 1; i < accesses.size(); i++) {
        int32_t diff = static_cast<int32_t>(accesses[i].address) - static_cast<int32_t>(accesses[i-1].address);
        strideCounts[diff]++;
    }
    
    // Find the most common stride
    auto maxStride = std::max_element(strideCounts.begin(), strideCounts.end(),
                                     [](const auto& a, const auto& b) { return a.second < b.second; });
    
    if (maxStride != strideCounts.end()) {
        confidence = static_cast<double>(maxStride->second) / (accesses.size() - 1);
        if (confidence > 0.6) { // 60% threshold for stride detection
            return maxStride->first;
        }
    }
    
    return std::nullopt;
}

bool TraceAnalyzer::isLoopingPattern(const std::vector<MemoryAccess>& accesses, size_t& loopSize, double& confidence) const {
    if (accesses.size() < 20) { // Need more data for loop detection
        confidence = 0.0;
        return false;
    }
    
    // Try different loop sizes
    for (size_t candidateSize = 3; candidateSize <= std::min<size_t>(accesses.size() / 3, 50); candidateSize++) {
        int matchCount = 0;
        int testCount = 0;
        
        // Test if the pattern repeats with this loop size
        for (size_t i = candidateSize; i < accesses.size(); i++) {
            if (i % candidateSize < accesses.size() - i) {
                testCount++;
                if (accesses[i].address == accesses[i % candidateSize].address) {
                    matchCount++;
                }
            }
        }
        
        if (testCount > 0) {
            double currentConfidence = static_cast<double>(matchCount) / testCount;
            if (currentConfidence > 0.8) { // 80% threshold for loop detection
                loopSize = candidateSize;
                confidence = currentConfidence;
                return true;
            }
        }
    }
    
    confidence = 0.0;
    return false;
}

std::vector<TraceStatistics::AddressRange> TraceAnalyzer::getHotRanges(size_t numRanges, uint32_t rangeSize) {
    std::vector<MemoryAccess> accesses = parser.parseAll();
    std::map<uint32_t, size_t> rangeAccesses;
    
    // Group accesses by range
    for (const auto& access : accesses) {
        uint32_t rangeStart = (access.address / rangeSize) * rangeSize;
        rangeAccesses[rangeStart]++;
    }
    
    // Convert to vector for sorting
    std::vector<std::pair<uint32_t, size_t>> rangeVector(rangeAccesses.begin(), rangeAccesses.end());
    
    // Sort by access count (descending)
    std::sort(rangeVector.begin(), rangeVector.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Create result vector with top ranges
    std::vector<TraceStatistics::AddressRange> result;
    for (size_t i = 0; i < std::min(numRanges, rangeVector.size()); i++) {
        const auto& [start, count] = rangeVector[i];
        result.push_back({start, start + rangeSize - 1, count});
    }
    
    return result;
}

void TraceAnalyzer::printAnalysis(const TraceStatistics& stats) const {
    std::cout << "Trace Analysis for " << parser.getFilepath() << std::endl;
    std::cout << "==================================================" << std::endl;
    
    // Basic statistics
    std::cout << "Total Accesses: " << stats.totalAccesses << std::endl;
    std::cout << "Read Accesses: " << stats.readAccesses << " (" 
              << std::fixed << std::setprecision(1)
              << (100.0 * stats.readAccesses / stats.totalAccesses) << "%)" << std::endl;
    std::cout << "Write Accesses: " << stats.writeAccesses << " (" 
              << std::fixed << std::setprecision(1)
              << (100.0 * stats.writeAccesses / stats.totalAccesses) << "%)" << std::endl;
    
    // Access pattern
    std::cout << std::endl << "Access Pattern Analysis:" << std::endl;
    if (stats.detectedPattern) {
        std::cout << "Detected Pattern: ";
        switch (stats.detectedPattern->type) {
            case TraceStatistics::PatternType::Sequential:
                std::cout << "Sequential";
                break;
            case TraceStatistics::PatternType::Strided:
                std::cout << "Strided (stride = " << stats.detectedPattern->stride << ")";
                break;
            case TraceStatistics::PatternType::LoopingAccess:
                std::cout << "Looping (loop size = " << stats.detectedPattern->loopSize << ")";
                break;
            case TraceStatistics::PatternType::Random:
                std::cout << "Random";
                break;
            case TraceStatistics::PatternType::Mixed:
                std::cout << "Mixed";
                break;
        }
        std::cout << " (confidence: " << std::fixed << std::setprecision(1)
                  << (100.0 * stats.detectedPattern->confidence) << "%)" << std::endl;
    } else {
        std::cout << "No clear pattern detected." << std::endl;
    }
    
    // Hot ranges
    std::cout << std::endl << "Hot Memory Regions:" << std::endl;
    if (stats.hotRanges.empty()) {
        std::cout << "No hot regions detected." << std::endl;
    } else {
        for (size_t i = 0; i < stats.hotRanges.size(); i++) {
            const auto& range = stats.hotRanges[i];
            std::cout << (i+1) << ". Address Range: 0x" << std::hex << range.start
                      << " - 0x" << range.end << std::dec
                      << " (" << range.accesses << " accesses, "
                      << std::fixed << std::setprecision(1)
                      << (100.0 * range.accesses / stats.totalAccesses) << "%)" << std::endl;
        }
    }
    
    // Unique addresses
    std::cout << std::endl << "Unique Addresses: " << stats.accessFrequency.size() << std::endl;
    
    // Find most accessed address
    uint32_t mostAccessedAddr = 0;
    size_t maxCount = 0;
    for (const auto& [addr, count] : stats.accessFrequency) {
        if (count > maxCount) {
            maxCount = count;
            mostAccessedAddr = addr;
        }
    }
    
    if (maxCount > 0) {
        std::cout << "Most Accessed Address: 0x" << std::hex << mostAccessedAddr
                  << std::dec << " (" << maxCount << " accesses, "
                  << std::fixed << std::setprecision(1)
                  << (100.0 * maxCount / stats.totalAccesses) << "%)" << std::endl;
    }
}

bool TraceAnalyzer::generateFilteredTrace(const std::filesystem::path& outputPath, 
                                         const std::function<bool(const MemoryAccess&)>& filter) {
    std::vector<MemoryAccess> accesses = parser.parseAll();
    std::ofstream outFile(outputPath);
    
    if (!outFile) {
        std::cerr << "Error: Could not create output file " << outputPath << std::endl;
        return false;
    }
    
    // Filter and write accesses
    for (const auto& access : accesses) {
        if (filter(access)) {
            outFile << (access.isWrite ? 'w' : 'r') << " 0x" 
                    << std::hex << access.address << std::dec << std::endl;
        }
    }
    
    return true;
}

namespace trace_generator {

// Helper function to write an access to file
inline void writeAccess(std::ofstream& file, bool isWrite, uint32_t address) {
    file << (isWrite ? 'w' : 'r') << " 0x" 
         << std::hex << address << std::dec << std::endl;
}

bool generateSequentialTrace(const std::filesystem::path& outputPath, 
                            uint32_t startAddress, 
                            size_t count, 
                            uint32_t stride,
                            double writeRatio) {
    std::ofstream file(outputPath);
    if (!file) {
        std::cerr << "Error: Could not create output file " << outputPath << std::endl;
        return false;
    }
    
    // Random number generator for write ratio
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Generate sequential accesses
    for (size_t i = 0; i < count; i++) {
        uint32_t address = startAddress + i * stride;
        bool isWrite = dis(gen) < writeRatio;
        writeAccess(file, isWrite, address);
    }
    
    return true;
}

bool generateRandomTrace(const std::filesystem::path& outputPath,
                        uint32_t minAddress,
                        uint32_t maxAddress,
                        size_t count,
                        double writeRatio) {
    std::ofstream file(outputPath);
    if (!file) {
        std::cerr << "Error: Could not create output file " << outputPath << std::endl;
        return false;
    }
    
    // Random generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> addrDis(minAddress, maxAddress);
    std::uniform_real_distribution<> writeDis(0.0, 1.0);
    
    // Generate random accesses
    for (size_t i = 0; i < count; i++) {
        uint32_t address = addrDis(gen);
        bool isWrite = writeDis(gen) < writeRatio;
        writeAccess(file, isWrite, address);
    }
    
    return true;
}

bool generateLocalityTrace(const std::filesystem::path& outputPath,
                          size_t numRegions,
                          uint32_t regionSize,
                          size_t accessesPerRegion,
                          double writeRatio) {
    std::ofstream file(outputPath);
    if (!file) {
        std::cerr << "Error: Could not create output file " << outputPath << std::endl;
        return false;
    }
    
    // Random generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> writeDis(0.0, 1.0);
    
    // Generate regions
    std::vector<uint32_t> regionStarts;
    for (size_t i = 0; i < numRegions; i++) {
        regionStarts.push_back(i * regionSize * 10); // Space out regions
    }
    
    // Generate accesses with spatial locality
    for (size_t region = 0; region < numRegions; region++) {
        uint32_t regionStart = regionStarts[region];
        
        // Generate accesses within this region
        for (size_t i = 0; i < accessesPerRegion; i++) {
            // Use a skewed distribution to favor lower offsets (better locality)
            std::exponential_distribution<> offsetDis(4.0 / regionSize);
            uint32_t offset = static_cast<uint32_t>(offsetDis(gen) * regionSize);
            offset = std::min(offset, regionSize - 1);
            
            uint32_t address = regionStart + offset;
            bool isWrite = writeDis(gen) < writeRatio;
            writeAccess(file, isWrite, address);
        }
    }
    
    return true;
}

bool generateMixedTrace(const std::filesystem::path& outputPath,
                       const std::vector<std::pair<std::string, double>>& patternMix,
                       size_t totalAccesses,
                       double writeRatio) {
    // Validate pattern mix ratios
    double totalRatio = 0.0;
    for (const auto& [pattern, ratio] : patternMix) {
        totalRatio += ratio;
    }
    
    if (std::abs(totalRatio - 1.0) > 0.001) {
        std::cerr << "Error: Pattern mix ratios must sum to 1.0" << std::endl;
        return false;
    }
    
    // Create temporary files for each pattern
    std::vector<std::filesystem::path> tempFiles;
    size_t patternIndex = 0;
    
    for (const auto& [pattern, ratio] : patternMix) {
        size_t patternCount = static_cast<size_t>(totalAccesses * ratio);
        auto tempPath = std::filesystem::temp_directory_path() / 
                       ("temp_pattern_" + std::to_string(patternIndex++) + ".txt");
        tempFiles.push_back(tempPath);
        
        if (pattern == "sequential") {
            generateSequentialTrace(tempPath, 0x1000, patternCount, 4, writeRatio);
        } else if (pattern == "random") {
            generateRandomTrace(tempPath, 0x1000, 0x1000000, patternCount, writeRatio);
        } else if (pattern == "locality") {
            generateLocalityTrace(tempPath, 5, 4096, patternCount / 5, writeRatio);
        } else {
            std::cerr << "Unknown pattern type: " << pattern << std::endl;
            return false;
        }
    }
    
    // Merge the temporary files into the output file
    std::ofstream outFile(outputPath);
    if (!outFile) {
        std::cerr << "Error: Could not create output file " << outputPath << std::endl;
        return false;
    }
    
    for (const auto& tempPath : tempFiles) {
        std::ifstream inFile(tempPath);
        if (inFile) {
            outFile << inFile.rdbuf();
        }
        
        // Clean up temp file
        std::filesystem::remove(tempPath);
    }
    
    return true;
}

} // namespace trace_generator

} // namespace cachesim