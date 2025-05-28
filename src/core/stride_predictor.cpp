#include "stride_predictor.h"
#include <algorithm>
#include <iomanip>

namespace cachesim {

StridePredictor::StridePredictor(int size)
    : table(size),
      correctPredictions(0),
      totalPredictions(0) {
}

void StridePredictor::update(uint32_t address) {
    uint32_t index = address % table.size();
    Entry& entry = table[index];
    
    if (entry.valid) {
        // Calculate the current stride
        int32_t currentStride = address - entry.lastAddress;
        
        // Check if stride is consistent
        if (currentStride == entry.stride) {
            // Stride matches prediction, increase confidence
            entry.confidence = std::min(entry.confidence + 1, 3);
            correctPredictions++;
        } else {
            // Stride pattern changed
            entry.lastStride = entry.stride;
            entry.stride = currentStride;
            entry.confidence = std::max(entry.confidence - 1, 0);
        }
        
        totalPredictions++;
        
        // Update stride histogram for statistics
        strideHistogram[currentStride]++;
    } else {
        // First access to this entry
        entry.valid = true;
        entry.stride = 0; // No stride yet
        entry.confidence = 0;
    }
    
    // Update last address
    entry.lastAddress = address;
}

int32_t StridePredictor::getStride(uint32_t address) const {
    uint32_t index = address % table.size();
    const Entry& entry = table[index];
    
    // Only return the stride if confidence is above threshold
    if (entry.valid && entry.confidence >= 2) {
        return entry.stride;
    }
    
    return 0; // No confident stride detected
}

double StridePredictor::getConfidence(uint32_t address) const {
    uint32_t index = address % table.size();
    const Entry& entry = table[index];
    
    if (!entry.valid) {
        return 0.0;
    }
    
    // Convert confidence level (0-3) to a percentage (0-100%)
    return (entry.confidence * 33.3);
}

double StridePredictor::getAccuracy() const {
    if (totalPredictions == 0) {
        return 0.0;
    }
    
    return static_cast<double>(correctPredictions) / totalPredictions;
}

void StridePredictor::resetStats() {
    correctPredictions = 0;
    totalPredictions = 0;
    strideHistogram.clear();
}

void StridePredictor::printStats() const {
    std::cout << "Stride Predictor Statistics:" << std::endl;
    std::cout << "  Table Size: " << table.size() << " entries" << std::endl;
    std::cout << "  Correct Predictions: " << correctPredictions << std::endl;
    std::cout << "  Total Predictions: " << totalPredictions << std::endl;
    std::cout << "  Prediction Accuracy: " << (getAccuracy() * 100.0) << "%" << std::endl;
    
    // Print stride histogram
    std::cout << "  Stride Histogram (top 10 strides):" << std::endl;
    
    // Sort strides by frequency
    std::vector<std::pair<int32_t, int>> sortedStrides;
    for (const auto& pair : strideHistogram) {
        sortedStrides.push_back(pair);
    }
    
    std::sort(sortedStrides.begin(), sortedStrides.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Print top 10 most frequent strides
    const int maxStridesToShow = std::min(10, static_cast<int>(sortedStrides.size()));
    for (int i = 0; i < maxStridesToShow; ++i) {
        std::cout << "    Stride " << std::setw(6) << sortedStrides[i].first
                  << ": " << sortedStrides[i].second << " occurrences" << std::endl;
    }
}

} // namespace cachesim