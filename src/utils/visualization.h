#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <functional>
#include <cmath>
#include <array>
#include <sstream>
#include <iomanip>

namespace cachesim {

/**
 * Visualization utilities for the cache simulator.
 * Provides methods to generate visual representations of cache behavior and statistics.
 */
class Visualization {
public:
    // Color constants for ASCII visualization
    struct Colors {
        static constexpr std::string_view Reset = "\033[0m";
        static constexpr std::string_view Red = "\033[31m";
        static constexpr std::string_view Green = "\033[32m";
        static constexpr std::string_view Yellow = "\033[33m";
        static constexpr std::string_view Blue = "\033[34m";
        static constexpr std::string_view Magenta = "\033[35m";
        static constexpr std::string_view Cyan = "\033[36m";
        static constexpr std::string_view White = "\033[37m";
        static constexpr std::string_view Bold = "\033[1m";
    };
    
    // Constructor
    Visualization() = default;
    
    // Check if terminal supports colors
    [[nodiscard]] static bool supportsColors() {
        #ifdef _WIN32
            // Check for Windows Terminal or ConEmu
            auto env = std::getenv("WT_SESSION");
            if (env && std::string(env).length() > 0) {
                return true;
            }
            
            env = std::getenv("ConEmuANSI");
            if (env && std::string(env) == "ON") {
                return true;
            }
            
            return false;
        #else
            // Check for TERM environment variable on Unix-like systems
            auto env = std::getenv("TERM");
            return env && std::string(env) != "dumb";
        #endif
    }
    
    // Disable colors
    static void disableColors() {
        colorEnabled = false;
    }
    
    // Enable colors
    static void enableColors() {
        colorEnabled = supportsColors();
    }
    
    // Apply color to a string
    [[nodiscard]] static std::string colorize(std::string_view text, std::string_view color) {
        if (!colorEnabled) {
            return std::string(text);
        }
        return std::string(color) + std::string(text) + std::string(Colors::Reset);
    }
    
    // Generate ASCII histogram
    [[nodiscard]] static std::string generateHistogram(const std::vector<std::pair<std::string, double>>& data, 
                                                    int width = 50, bool useColors = true) {
        if (data.empty()) {
            return "No data to visualize.";
        }
        
        // Find maximum value for scaling
        double maxValue = 0.0;
        for (const auto& [_, value] : data) {
            maxValue = std::max(maxValue, value);
        }
        
        // Calculate scale factor
        double scaleFactor = maxValue > 0 ? static_cast<double>(width) / maxValue : 0;
        
        std::stringstream ss;
        
        // Find the longest label for proper alignment
        size_t maxLabelLength = 0;
        for (const auto& [label, _] : data) {
            maxLabelLength = std::max(maxLabelLength, label.length());
        }
        
        // Draw histogram
        for (const auto& [label, value] : data) {
            int barLength = static_cast<int>(std::round(value * scaleFactor));
            barLength = std::max(barLength, 1); // Ensure at least 1 character
            
            // Format label with padding
            ss << std::left << std::setw(maxLabelLength + 2) << label;
            
            // Format value
            ss << std::right << std::setw(8) << std::fixed << std::setprecision(2) << value << " │ ";
            
            // Draw bar
            std::string bar(barLength, '#'); // Changed '█' to '#'
            
            if (useColors && colorEnabled) {
                // Choose color based on value (gradient from blue to red)
                std::string_view color;
                double ratio = value / maxValue;
                
                if (ratio < 0.25) color = Colors::Blue;
                else if (ratio < 0.5) color = Colors::Cyan;
                else if (ratio < 0.75) color = Colors::Yellow;
                else color = Colors::Red;
                
                ss << colorize(bar, color);
            } else {
                ss << bar;
            }
            
            ss << std::endl;
        }
        
        return ss.str();
    }
    
    // Generate ASCII heatmap for 2D data
    [[nodiscard]] static std::string generateHeatmap(const std::vector<std::vector<double>>& data, 
                                                  const std::vector<std::string>& rowLabels = {},
                                                  const std::vector<std::string>& colLabels = {},
                                                  bool useColors = true) {
        if (data.empty() || data[0].empty()) {
            return "No data to visualize.";
        }
        
        // Find min and max values
        double minValue = std::numeric_limits<double>::max();
        double maxValue = std::numeric_limits<double>::lowest();
        
        for (const auto& row : data) {
            for (const auto& value : row) {
                minValue = std::min(minValue, value);
                maxValue = std::max(maxValue, value);
            }
        }
        
        // Define characters for gradient
        static const std::vector<char> gradientChars = { ' ', '.', '+', '-', '=', '@', '#' }; // Changed characters
        
        // Define colors for gradient
        static const std::array<std::string_view, 6> gradientColors = {
            Colors::Blue, Colors::Cyan, Colors::Green, 
            Colors::Yellow, Colors::Magenta, Colors::Red
        };
        
        std::stringstream ss;
        
        // Calculate column widths
        std::vector<size_t> colWidths;
        
        // Start with width based on value formatting
        size_t valueWidth = 6; // Fixed width for values
        
        // Adjust if column labels are present and wider
        if (!colLabels.empty()) {
            colWidths.resize(colLabels.size());
            for (size_t i = 0; i < colLabels.size(); ++i) {
                colWidths[i] = std::max(valueWidth, colLabels[i].length());
            }
        } else {
            colWidths.resize(data[0].size(), valueWidth);
        }
        
        // Draw column headers if present
        if (!colLabels.empty()) {
            // Find max row label width
            size_t rowLabelWidth = 0;
            for (const auto& label : rowLabels) {
                rowLabelWidth = std::max(rowLabelWidth, label.length());
            }
            
            // Print column headers
            ss << std::string(rowLabelWidth + 2, ' '); // Padding for row labels
            
            for (size_t i = 0; i < colLabels.size() && i < data[0].size(); ++i) {
                ss << std::left << std::setw(colWidths[i] + 1) << colLabels[i];
            }
            ss << std::endl;
            
            // Print separator
            ss << std::string(rowLabelWidth + 2, ' ');
            for (size_t i = 0; i < data[0].size(); ++i) {
                ss << std::string(colWidths[i], '-') << ' ';
            }
            ss << std::endl;
        }
        
        // Draw heatmap
        for (size_t i = 0; i < data.size(); ++i) {
            // Print row label if present
            if (i < rowLabels.size()) {
                ss << std::left << std::setw(rowLabels[i].length() + 2) << rowLabels[i];
            } else {
                ss << std::left << std::setw(5) << i << ' ';
            }
            
            // Print values
            for (size_t j = 0; j < data[i].size(); ++j) {
                double value = data[i][j];
                double normalizedValue = (maxValue > minValue) ? 
                    (value - minValue) / (maxValue - minValue) : 0.5;
                
                // Choose character from gradient
                int charIndex = static_cast<int>(normalizedValue * (gradientChars.size() - 1));
                char gradientChar = gradientChars[charIndex];
                
                // Format value
                std::stringstream cellSs;
                cellSs << std::fixed << std::setprecision(2) << value;
                std::string cell = cellSs.str();
                
                // Pad to column width
                while (cell.length() < colWidths[j]) {
                    cell += ' ';
                }
                
                // Add gradient character
                cell += gradientChar;
                
                // Apply color if enabled
                if (useColors && colorEnabled) {
                    int colorIndex = static_cast<int>(normalizedValue * (gradientColors.size() - 1));
                    ss << colorize(cell, gradientColors[colorIndex]);
                } else {
                    ss << cell;
                }
                
                ss << ' ';
            }
            
            ss << std::endl;
        }
        
        return ss.str();
    }
    
    // Generate cache state visualization
    template<typename CacheType>
    [[nodiscard]] static std::string visualizeCacheState(const CacheType& cache, 
                                                     int maxSets = 16, 
                                                     bool useColors = true) {
        std::stringstream ss;
        
        // Get cache parameters
        int numSets = cache.getNumSets();
        int associativity = cache.getBlocksPerSet();
        
        // Limit the number of sets to display
        int setsToShow = std::min(numSets, maxSets);
        
        ss << "Cache State Visualization" << std::endl;
        ss << "=========================" << std::endl;
        ss << "Sets: " << numSets << ", Ways: " << associativity << std::endl;
        
        if (numSets > maxSets) {
            ss << "(Showing first " << maxSets << " sets)" << std::endl;
        }
        
        ss << std::endl;
        
        // Header
        ss << "    │ ";
        for (int way = 0; way < associativity; ++way) {
            ss << "Way " << std::setw(2) << way << " │ ";
        }
        ss << std::endl;
        
        ss << "----+";
        for (int way = 0; way < associativity; ++way) {
            ss << "--------+";
        }
        ss << std::endl;
        
        // Show each set
        for (int set = 0; set < setsToShow; ++set) {
            ss << std::setw(3) << set << " │ ";
            
            // Get blocks in this set
            auto blocks = cache.getSetState(set);
            
            for (int way = 0; way < associativity; ++way) {
                // Check if we have block data
                if (way < blocks.size()) {
                    auto& block = blocks[way];
                    
                    // Create a representation of the block state
                    std::string blockState;
                    
                    if (block.valid) {
                        // Convert tag to block address
                        std::stringstream tagSs;
                        tagSs << std::hex << std::setw(4) << std::setfill('0') << block.tag;
                        
                        blockState = tagSs.str();
                        
                        // Add state indicators
                        if (block.dirty) {
                            blockState += " D";
                        } else {
                            blockState += " -";
                        }
                        
                        // Color based on state
                        if (useColors && colorEnabled) {
                            if (block.mesiState == MESIState::Modified) {
                                blockState = colorize(blockState, Colors::Red);
                            } else if (block.mesiState == MESIState::Exclusive) {
                                blockState = colorize(blockState, Colors::Green);
                            } else if (block.mesiState == MESIState::Shared) {
                                blockState = colorize(blockState, Colors::Blue);
                            }
                        }
                    } else {
                        blockState = "  --  ";
                    }
                    
                    ss << blockState << " │ ";
                } else {
                    ss << "  --   │ ";
                }
            }
            
            ss << std::endl;
        }
        
        return ss.str();
    }
    
    // Generate memory access pattern visualization
    [[nodiscard]] static std::string visualizeAccessPattern(const std::vector<uint32_t>& addresses, 
                                                        int width = 80, 
                                                        int height = 20, 
                                                        bool useColors = true) {
        if (addresses.empty()) {
            return "No addresses to visualize.";
        }
        
        // Find min and max addresses
        uint32_t minAddr = *std::min_element(addresses.begin(), addresses.end());
        uint32_t maxAddr = *std::max_element(addresses.begin(), addresses.end());
        
        // Avoid division by zero
        if (minAddr == maxAddr) {
            maxAddr = minAddr + 1;
        }
        
        // Create a 2D grid
        std::vector<std::vector<int>> grid(height, std::vector<int>(width, 0));
        
        // Map addresses to grid
        for (uint32_t addr : addresses) {
            // Normalize address to [0, 1]
            double normalizedAddr = static_cast<double>(addr - minAddr) / (maxAddr - minAddr);
            
            // Map to grid coordinates
            int x = static_cast<int>(normalizedAddr * (width - 1));
            int y = height - 1; // Start from bottom
            
            // Find first empty row from bottom
            while (y >= 0 && grid[y][x] > 0) {
                --y;
            }
            
            // Increment counter at this position
            if (y >= 0) {
                grid[y][x]++;
            }
        }
        
        // Find maximum count for color scaling
        int maxCount = 0;
        for (const auto& row : grid) {
            for (int count : row) {
                maxCount = std::max(maxCount, count);
            }
        }
        
        // Generate visualization
        std::stringstream ss;
        
        ss << "Memory Access Pattern Visualization" << std::endl;
        ss << "==================================" << std::endl;
        ss << "Address Range: 0x" << std::hex << minAddr << " - 0x" << maxAddr << std::dec << std::endl;
        ss << "Total Accesses: " << addresses.size() << std::endl;
        ss << std::endl;
        
        // Draw grid
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int count = grid[y][x];
                
                // Choose character based on count
                char c;
                if (count == 0) c = ' ';
                else if (count <= maxCount / 4) c = '.';
                else if (count <= maxCount / 2) c = '+';
                else if (count <= 3 * maxCount / 4) c = '=';
                else c = '#';
                
                // Apply color if enabled
                if (useColors && colorEnabled && count > 0) {
                    // Calculate color based on count
                    double ratio = static_cast<double>(count) / maxCount;
                    std::string_view color;
                    
                    if (ratio < 0.25) color = Colors::Blue;
                    else if (ratio < 0.5) color = Colors::Cyan;
                    else if (ratio < 0.75) color = Colors::Yellow;
                    else color = Colors::Red;
                    
                    ss << colorize(std::string(1, c), color);
                } else {
                    ss << c;
                }
            }
            ss << std::endl;
        }
        
        // Draw X-axis labels (using ASCII alternatives)
        ss << "L";
        ss << std::string(width - 2, '-');
        ss << "J" << std::endl;
        
        ss << std::hex << minAddr;
        ss << std::string(width - 20, ' ');
        ss << maxAddr << std::dec << std::endl;
        
        return ss.str();
    }
    
    // Export data to CSV for external visualization
    template<typename DataType>
    static bool exportToCsv(const std::filesystem::path& filePath, 
                          const std::vector<std::pair<std::string, DataType>>& data) {
        std::ofstream file(filePath);
        if (!file) {
            return false;
        }
        
        // Write header
        file << "Label,Value" << std::endl;
        
        // Write data
        for (const auto& [label, value] : data) {
            file << label << "," << value << std::endl;
        }
        
        return true;
    }
    
    // Export 2D data to CSV
    template<typename DataType>
    static bool exportToCsv(const std::filesystem::path& filePath,
                          const std::vector<std::vector<DataType>>& data,
                          const std::vector<std::string>& rowLabels = {},
                          const std::vector<std::string>& colLabels = {}) {
        std::ofstream file(filePath);
        if (!file) {
            return false;
        }
        
        // Write header with column labels
        if (!colLabels.empty()) {
            file << ","; // Empty cell for row labels column
            for (size_t i = 0; i < colLabels.size() && i < data[0].size(); ++i) {
                file << colLabels[i];
                if (i < colLabels.size() - 1) {
                    file << ",";
                }
            }
            file << std::endl;
        }
        
        // Write data with row labels
        for (size_t i = 0; i < data.size(); ++i) {
            // Write row label if available
            if (i < rowLabels.size()) {
                file << rowLabels[i] << ",";
            } else {
                file << "Row " << i << ",";
            }
            
            // Write values
            for (size_t j = 0; j < data[i].size(); ++j) {
                file << data[i][j];
                if (j < data[i].size() - 1) {
                    file << ",";
                }
            }
            file << std::endl;
        }
        
        return true;
    }

private:
    static bool colorEnabled;
};

// Initialize static member
bool Visualization::colorEnabled = Visualization::supportsColors();

} // namespace cachesim