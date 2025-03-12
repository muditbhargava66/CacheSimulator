#pragma once

#include <cmath>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <optional>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <filesystem>
#include <variant>
#include <functional>
#include <set>

namespace cachesim {

/**
 * Utility class for statistical analysis of simulation results.
 * Implements various statistical operations for analyzing cache performance.
 */
class Statistics {
public:
    // Metric type to store different kinds of statistical data
    using Metric = std::variant<double, int, std::string, bool>;
    
    // Constructor
    Statistics() = default;
    
    // Add or update a metric
    template<typename T>
    void addMetric(std::string_view name, T value) {
        // Convert to one of the variant types
        if constexpr (std::is_integral_v<T>) {
            // Convert any integer type to int
            metrics[std::string(name)] = static_cast<int>(value);
        } else if constexpr (std::is_floating_point_v<T>) {
            // Convert any floating point to double
            metrics[std::string(name)] = static_cast<double>(value);
        } else if constexpr (std::is_same_v<T, std::string> || 
                            std::is_same_v<T, const char*> ||
                            std::is_same_v<T, std::string_view>) {
            // Convert any string type to std::string
            metrics[std::string(name)] = std::string(value);
        } else if constexpr (std::is_same_v<T, bool>) {
            // Boolean stays as is
            metrics[std::string(name)] = value;
        } else {
            // For other types, convert to string
            std::ostringstream ss;
            ss << value;
            metrics[std::string(name)] = ss.str();
        }
    }
    
    // Get a metric (typed)
    template<typename T>
    [[nodiscard]] std::optional<T> getMetric(std::string_view name) const {
        auto it = metrics.find(std::string(name));
        if (it != metrics.end()) {
            if (const T* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        return std::nullopt;
    }
    
    // Get all metrics
    [[nodiscard]] const auto& getAllMetrics() const { 
        return metrics; 
    }
    
    // Clear all metrics
    void clear() { 
        metrics.clear(); 
    }
    
    // Calculate improvement percentage between two values
    [[nodiscard]] static double calculateImprovement(double baseline, double current) {
        if (baseline == 0) {
            return (current == 0) ? 0.0 : std::numeric_limits<double>::infinity();
        }
        return ((baseline - current) / baseline) * 100.0;
    }
    
    // Calculate relative performance between two values
    [[nodiscard]] static double calculateRelativePerformance(double baseline, double current) {
        if (current == 0) {
            return std::numeric_limits<double>::infinity();
        }
        return baseline / current;
    }
    
    // Calculate basic statistics for a vector of values
    template<typename T>
    [[nodiscard]] static std::unordered_map<std::string, double> calculateStats(const std::vector<T>& values) {
        if (values.empty()) {
            return {};
        }
        
        std::unordered_map<std::string, double> stats;
        
        // Calculate min, max, mean
        auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
        stats["min"] = static_cast<double>(*minIt);
        stats["max"] = static_cast<double>(*maxIt);
        
        double sum = std::accumulate(values.begin(), values.end(), 0.0,
                                    [](double acc, const T& val) { return acc + static_cast<double>(val); });
        stats["mean"] = sum / values.size();
        
        // Calculate median
        std::vector<double> sortedValues;
        sortedValues.reserve(values.size());
        for (const auto& val : values) {
            sortedValues.push_back(static_cast<double>(val));
        }
        std::sort(sortedValues.begin(), sortedValues.end());
        
        if (sortedValues.size() % 2 == 0) {
            stats["median"] = (sortedValues[sortedValues.size() / 2 - 1] + 
                              sortedValues[sortedValues.size() / 2]) / 2.0;
        } else {
            stats["median"] = sortedValues[sortedValues.size() / 2];
        }
        
        // Calculate standard deviation
        double variance = 0.0;
        for (const auto& val : values) {
            double diff = static_cast<double>(val) - stats["mean"];
            variance += diff * diff;
        }
        variance /= values.size();
        stats["stddev"] = std::sqrt(variance);
        
        // Calculate coefficient of variation
        if (stats["mean"] != 0) {
            stats["cv"] = (stats["stddev"] / stats["mean"]) * 100.0;
        } else {
            stats["cv"] = 0.0;
        }
        
        // Calculate percentiles (25th and 75th)
        size_t p25_idx = values.size() / 4;
        size_t p75_idx = values.size() * 3 / 4;
        stats["p25"] = sortedValues[p25_idx];
        stats["p75"] = sortedValues[p75_idx];
        
        return stats;
    }
    
    // Print a summary of all metrics
    void printSummary(std::ostream& out = std::cout) const {
        out << "Statistics Summary" << std::endl;
        out << "===================" << std::endl;
        
        // Group metrics by category
        std::unordered_map<std::string, std::vector<std::pair<std::string, Metric>>> categories;
        
        for (const auto& [name, value] : metrics) {
            // Extract category (part before .)
            std::string category = "General";
            size_t dotPos = name.find('.');
            if (dotPos != std::string::npos) {
                category = name.substr(0, dotPos);
            }
            
            categories[category].push_back({name, value});
        }
        
        // Print metrics by category
        for (const auto& [category, metricsGroup] : categories) {
            out << category << ":" << std::endl;
            out << std::string(category.length() + 1, '-') << std::endl;
            
            for (const auto& [name, value] : metricsGroup) {
                std::string metricName = name;
                if (metricName.find('.') != std::string::npos) {
                    metricName = metricName.substr(metricName.find('.') + 1);
                }
                
                out << "  " << std::left << std::setw(30) << metricName << ": ";
                
                // Print different types of metrics appropriately
                std::visit([&out](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, double>) {
                        out << std::fixed << std::setprecision(4) << v;
                    } else if constexpr (std::is_same_v<T, int>) {
                        out << v;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        out << v;
                    } else if constexpr (std::is_same_v<T, bool>) {
                        out << (v ? "true" : "false");
                    }
                }, value);
                
                out << std::endl;
            }
            out << std::endl;
        }
    }
    
    // Export metrics to CSV file
    bool exportToCsv(const std::filesystem::path& filePath) const {
        std::ofstream file(filePath);
        if (!file) {
            return false;
        }
        
        // Write header
        file << "Metric,Value" << std::endl;
        
        // Write metrics
        for (const auto& [name, value] : metrics) {
            file << name << ",";
            
            // Write value based on type
            std::visit([&file](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>) {
                    file << std::fixed << std::setprecision(6) << v;
                } else if constexpr (std::is_same_v<T, int>) {
                    file << v;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    // Escape commas in strings
                    std::string escaped = v;
                    if (escaped.find(',') != std::string::npos) {
                        file << "\"" << escaped << "\"";
                    } else {
                        file << escaped;
                    }
                } else if constexpr (std::is_same_v<T, bool>) {
                    file << (v ? "true" : "false");
                }
            }, value);
            
            file << std::endl;
        }
        
        return true;
    }
    
    // Compare two sets of statistics
    static void compareStats(const Statistics& baseline, const Statistics& current, 
                           std::ostream& out = std::cout) {
        out << "Statistics Comparison" << std::endl;
        out << "=====================" << std::endl;
        
        // Find all unique metric names
        std::set<std::string> allMetrics;
        for (const auto& [name, _] : baseline.metrics) {
            allMetrics.insert(name);
        }
        for (const auto& [name, _] : current.metrics) {
            allMetrics.insert(name);
        }
        
        // Print comparison for each metric
        for (const auto& name : allMetrics) {
            // Skip strings and booleans
            auto baselineValue = baseline.getMetric<double>(name);
            auto currentValue = current.getMetric<double>(name);
            
            if (!baselineValue && !currentValue) {
                baselineValue = baseline.getMetric<int>(name);
                currentValue = current.getMetric<int>(name);
                
                if (!baselineValue && !currentValue) {
                    // Skip non-numeric metrics
                    continue;
                }
            }
            
            out << std::left << std::setw(30) << name << ": ";
            
            if (baselineValue && currentValue) {
                // Calculate improvement
                double improvement = calculateImprovement(*baselineValue, *currentValue);
                
                out << std::fixed << std::setprecision(4) << *baselineValue 
                    << " -> " << *currentValue << " ("
                    << std::showpos << std::setprecision(2) << improvement << "%"
                    << std::noshowpos << ")";
            } else if (baselineValue) {
                out << std::fixed << std::setprecision(4) << *baselineValue 
                    << " -> [not present]";
            } else if (currentValue) {
                out << "[not present] -> " << std::fixed << std::setprecision(4) 
                    << *currentValue;
            }
            
            out << std::endl;
        }
    }

private:
    std::unordered_map<std::string, Metric> metrics;
};

} // namespace cachesim