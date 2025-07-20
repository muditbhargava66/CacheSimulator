#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <iostream>

namespace cachesim {

// StridePredictor class for detecting memory access patterns with constant strides
class StridePredictor {
public:
    // Constructor
    StridePredictor(int size);
    
    // Update stride prediction table with a new address
    void update(uint32_t address);
    
    // Get the predicted stride for an address
    [[nodiscard]] int32_t getStride(uint32_t address) const;
    
    // Check confidence level of the predicted stride
    [[nodiscard]] double getConfidence(uint32_t address) const;
    
    // Statistics
    void printStats() const;
    void resetStats();
    [[nodiscard]] int getCorrectPredictions() const { return correctPredictions; }
    [[nodiscard]] int getTotalPredictions() const { return totalPredictions; }
    [[nodiscard]] double getAccuracy() const;

private:
    struct Entry {
        uint32_t lastAddress = 0;     // Last address seen
        int32_t stride = 0;           // Detected stride
        int32_t lastStride = 0;       // Previous stride
        int confidence = 0;           // Confidence level (0-3)
        bool valid = false;           // Is the entry valid?
    };
    
    std::vector<Entry> table;         // Stride prediction table
    
    // Statistics
    int correctPredictions;           // Number of correct predictions
    int totalPredictions;             // Total number of predictions made
    std::unordered_map<int32_t, int> strideHistogram; // Histogram of detected strides
};

} // namespace cachesim