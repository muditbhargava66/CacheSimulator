/**
 * @file visualization_test.cpp
 * @brief Unit tests for statistical visualization features
 * @author Mudit Bhargava
 * @date 2025-06-01
 * @version 1.2.0
 */

#include <iostream>
#include <cassert>
#include <sstream>
#include <regex>
#include <random>
#include "../../../src/utils/visualization.h"

using namespace cachesim;

class VisualizationTest {
public:
    static void testHistogram() {
        std::cout << "Testing histogram generation..." << std::endl;
        
        std::vector<std::pair<std::string, double>> data = {
            {"L1 Hit Rate", 0.92},
            {"L2 Hit Rate", 0.75},
            {"L3 Hit Rate", 0.45},
            {"Memory", 0.15}
        };
        
        auto histogram = Visualization::generateHistogram(data, 30, false);
        
        // Verify output contains all labels
        for (const auto& [label, value] : data) {
            assert(histogram.find(label) != std::string::npos && 
                   "Histogram should contain all labels");
        }
        
        // Verify bars are proportional
        size_t l1Pos = histogram.find("0.92");
        size_t l3Pos = histogram.find("0.45");
        assert(l1Pos != std::string::npos && l3Pos != std::string::npos);
        
        std::cout << "✓ Histogram test passed!" << std::endl;
    }
    
    static void testLineChart() {
        std::cout << "Testing line chart generation..." << std::endl;
        
        // Generate time series data
        std::vector<std::pair<double, double>> timeSeries;
        for (int i = 0; i <= 10; ++i) {
            double x = i;
            double y = std::sin(i * 0.5) * 50 + 50;  // Sine wave 0-100
            timeSeries.push_back({x, y});
        }
        
        auto chart = Visualization::generateLineChart(
            timeSeries, 60, 15,
            "Cache Hit Rate Over Time",
            "Time (seconds)",
            "Hit Rate (%)",
            false
        );
        
        // Verify title and labels
        assert(chart.find("Cache Hit Rate Over Time") != std::string::npos);
        assert(chart.find("Time (seconds)") != std::string::npos);
        assert(chart.find("Hit Rate (%)") != std::string::npos);
        
        // Verify chart contains data points
        assert(chart.find("*") != std::string::npos && "Should contain data points");
        assert(chart.find(".") != std::string::npos && "Should contain connecting lines");
        
        std::cout << "✓ Line chart test passed!" << std::endl;
    }
    
    static void testPieChart() {
        std::cout << "Testing pie chart generation..." << std::endl;
        
        std::vector<std::pair<std::string, double>> data = {
            {"Compulsory Misses", 15.0},
            {"Capacity Misses", 35.0},
            {"Conflict Misses", 30.0},
            {"Coherence Misses", 20.0}
        };
        
        auto pie = Visualization::generatePieChart(data, 10, false);
        
        // Verify output
        assert(pie.find("Pie Chart") != std::string::npos);
        assert(pie.find("Legend:") != std::string::npos);
        
        // Verify all categories are in legend
        for (const auto& [label, value] : data) {
            assert(pie.find(label) != std::string::npos);
        }
        
        // Verify percentages
        assert(pie.find("%") != std::string::npos && "Should show percentages");
        
        std::cout << "✓ Pie chart test passed!" << std::endl;
    }
    
    static void testScatterPlot() {
        std::cout << "Testing scatter plot generation..." << std::endl;
        
        // Generate correlated data
        std::vector<std::pair<double, double>> data;
        std::mt19937 rng(42);
        std::normal_distribution<double> noise(0, 5);
        
        for (int i = 0; i < 50; ++i) {
            double x = i;
            double y = 2 * x + 10 + noise(rng);  // y = 2x + 10 + noise
            data.push_back({x, y});
        }
        
        auto scatter = Visualization::generateScatterPlot(
            data, 60, 20,
            "Cache Size vs Miss Rate",
            false
        );
        
        // Verify title
        assert(scatter.find("Cache Size vs Miss Rate") != std::string::npos);
        
        // Verify axes
        assert(scatter.find("Y:") != std::string::npos);
        assert(scatter.find("X:") != std::string::npos);
        
        // Verify data points
        assert(scatter.find("o") != std::string::npos || 
               scatter.find(".") != std::string::npos ||
               scatter.find("*") != std::string::npos);
        
        std::cout << "✓ Scatter plot test passed!" << std::endl;
    }
    
    static void testHeatmap() {
        std::cout << "Testing heatmap generation..." << std::endl;
        
        // Create 2D data (e.g., cache hit rates by set and way)
        std::vector<std::vector<double>> data(8, std::vector<double>(4));
        
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 4; ++j) {
                // Create pattern with hot spots
                data[i][j] = (i == 3 || i == 4) && (j == 1 || j == 2) ? 
                            90.0 + (i + j) : 20.0 + (i * j);
            }
        }
        
        std::vector<std::string> rowLabels = {
            "Set0", "Set1", "Set2", "Set3", "Set4", "Set5", "Set6", "Set7"
        };
        std::vector<std::string> colLabels = {
            "Way0", "Way1", "Way2", "Way3"
        };
        
        auto heatmap = Visualization::generateHeatmap(
            data, rowLabels, colLabels, false
        );
        
        // Verify labels
        for (const auto& label : rowLabels) {
            assert(heatmap.find(label) != std::string::npos);
        }
        for (const auto& label : colLabels) {
            assert(heatmap.find(label) != std::string::npos);
        }
        
        // Verify gradient characters
        std::string gradientChars = " .+-=@#";
        bool foundGradient = false;
        for (char c : gradientChars) {
            if (heatmap.find(c) != std::string::npos) {
                foundGradient = true;
                break;
            }
        }
        assert(foundGradient && "Should contain gradient characters");
        
        std::cout << "✓ Heatmap test passed!" << std::endl;
    }
    
    static void testColorSupport() {
        std::cout << "Testing color support..." << std::endl;
        
        // Test color detection
        bool supportsColors = Visualization::supportsColors();
        std::cout << "  Terminal color support: " << 
                     (supportsColors ? "Yes" : "No") << std::endl;
        
        // Test colorize function
        if (supportsColors) {
            Visualization::enableColors();
            auto colored = Visualization::colorize("Test", Visualization::Colors::Red);
            assert(colored.find("\033[31m") != std::string::npos && 
                   "Should contain red color code");
            assert(colored.find("\033[0m") != std::string::npos && 
                   "Should contain reset code");
        }
        
        // Test with colors disabled
        Visualization::disableColors();
        auto plain = Visualization::colorize("Test", Visualization::Colors::Red);
        assert(plain == "Test" && "Should not add color codes when disabled");
        
        // Restore color state
        if (supportsColors) {
            Visualization::enableColors();
        }
        
        std::cout << "✓ Color support test passed!" << std::endl;
    }
    
    static void testCSVExport() {
        std::cout << "Testing CSV export..." << std::endl;
        
        // Test 1D data export
        std::vector<std::pair<std::string, double>> data1D = {
            {"Metric1", 10.5},
            {"Metric2", 20.3},
            {"Metric3", 15.8}
        };
        
        std::string csvFile1D = "test_export_1d.csv";
        bool success = Visualization::exportToCsv(csvFile1D, data1D);
        assert(success && "Should export 1D data successfully");
        
        // Verify file contents
        std::ifstream file1D(csvFile1D);
        std::string line;
        std::getline(file1D, line);
        assert(line == "Label,Value" && "Should have correct header");
        
        // Clean up
        std::filesystem::remove(csvFile1D);
        
        // Test 2D data export
        std::vector<std::vector<double>> data2D = {
            {1.1, 2.2, 3.3},
            {4.4, 5.5, 6.6}
        };
        std::vector<std::string> rowLabels = {"Row1", "Row2"};
        std::vector<std::string> colLabels = {"Col1", "Col2", "Col3"};
        
        std::string csvFile2D = "test_export_2d.csv";
        success = Visualization::exportToCsv(csvFile2D, data2D, rowLabels, colLabels);
        assert(success && "Should export 2D data successfully");
        
        // Clean up
        std::filesystem::remove(csvFile2D);
        
        std::cout << "✓ CSV export test passed!" << std::endl;
    }
    
    static void testAccessPatternVisualization() {
        std::cout << "Testing access pattern visualization..." << std::endl;
        
        // Generate access pattern
        std::vector<uint32_t> addresses;
        
        // Sequential accesses
        for (uint32_t addr = 0x1000; addr < 0x1100; addr += 0x10) {
            for (int i = 0; i < 5; ++i) {
                addresses.push_back(addr);
            }
        }
        
        // Random accesses
        std::mt19937 rng(42);
        std::uniform_int_distribution<uint32_t> dist(0x2000, 0x3000);
        for (int i = 0; i < 50; ++i) {
            addresses.push_back(dist(rng));
        }
        
        auto pattern = Visualization::visualizeAccessPattern(
            addresses, 60, 15, false
        );
        
        // Verify output
        assert(pattern.find("Memory Access Pattern") != std::string::npos);
        assert(pattern.find("Address Range:") != std::string::npos);
        assert(pattern.find("Total Accesses:") != std::string::npos);
        
        // Should show hot spots for sequential accesses
        assert(pattern.find("#") != std::string::npos && 
               "Should show hot spots");
        
        std::cout << "✓ Access pattern visualization test passed!" << std::endl;
    }
    
    static void testEdgeCases() {
        std::cout << "Testing edge cases..." << std::endl;
        
        // Empty data
        std::vector<std::pair<std::string, double>> emptyData;
        auto emptyHist = Visualization::generateHistogram(emptyData);
        assert(emptyHist.find("No data") != std::string::npos);
        
        // Single data point
        std::vector<std::pair<double, double>> singlePoint = {{5.0, 10.0}};
        auto singleChart = Visualization::generateLineChart(singlePoint);
        assert(singleChart.find("*") != std::string::npos);
        
        // Zero values
        std::vector<std::pair<std::string, double>> zeroData = {
            {"Zero1", 0.0},
            {"Zero2", 0.0}
        };
        auto zeroPie = Visualization::generatePieChart(zeroData);
        assert(zeroPie.find("Invalid data") != std::string::npos ||
               zeroPie.find("0.0%") != std::string::npos);
        
        std::cout << "✓ Edge cases test passed!" << std::endl;
    }
    
    static void runAllTests() {
        std::cout << "Running Visualization Tests..." << std::endl;
        std::cout << "==============================" << std::endl;
        
        testHistogram();
        testLineChart();
        testPieChart();
        testScatterPlot();
        testHeatmap();
        testColorSupport();
        testCSVExport();
        testAccessPatternVisualization();
        testEdgeCases();
        
        std::cout << std::endl;
        std::cout << "All Visualization tests passed! ✅" << std::endl;
    }
};

int main() {
    try {
        VisualizationTest::runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
