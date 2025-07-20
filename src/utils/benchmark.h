#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cmath>
#include <limits>

namespace cachesim {

/**
 * Benchmarking utilities for measuring performance of the cache simulator.
 * Uses C++17 features like std::string_view, structured bindings, and std::optional.
 */
class Benchmark {
public:
    // Result of a single benchmark run
    struct Result {
        std::string name;
        std::chrono::nanoseconds duration;
        std::optional<double> value;  // Optional metric value
        std::unordered_map<std::string, double> metrics;
        
        // C++17 conversion to milliseconds
        [[nodiscard]] double milliseconds() const {
            return std::chrono::duration<double, std::milli>(duration).count();
        }
        
        // C++17 conversion to seconds
        [[nodiscard]] double seconds() const {
            return std::chrono::duration<double>(duration).count();
        }
    };
    
    // Run a benchmark once
    template<typename Func, typename... Args>
    static Result run(std::string_view name, Func&& func, Args&&... args) {
        Result result;
        result.name = std::string(name);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        if constexpr (std::is_invocable_r_v<double, Func, Args...>) {
            // Function returns a value (to prevent optimization)
            result.value = std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        } else {
            // Function doesn't return a value
            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        return result;
    }
    
    // Run a benchmark multiple times
    template<typename Func, typename... Args>
    static std::vector<Result> runMultiple(std::string_view name, size_t iterations, 
                                        bool warmup, Func&& func, Args&&... args) {
        std::vector<Result> results;
        results.reserve(iterations);
        
        // Optional warmup run (not included in results)
        if (warmup) {
            std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
        }
        
        // Run the benchmark multiple times
        for (size_t i = 0; i < iterations; ++i) {
            auto iterName = std::string(name) + " (iteration " + std::to_string(i + 1) + ")";
            results.push_back(run(iterName, std::forward<Func>(func), std::forward<Args>(args)...));
        }
        
        return results;
    }
    
    // Calculate statistics from benchmark results
    static std::unordered_map<std::string, double> calculateStats(const std::vector<Result>& results) {
        if (results.empty()) {
            return {};
        }
        
        std::unordered_map<std::string, double> stats;
        
        // Extract durations in milliseconds
        std::vector<double> durations;
        durations.reserve(results.size());
        
        for (const auto& result : results) {
            durations.push_back(result.milliseconds());
        }
        
        // Calculate min, max, mean
        auto [minIt, maxIt] = std::minmax_element(durations.begin(), durations.end());
        stats["min_ms"] = *minIt;
        stats["max_ms"] = *maxIt;
        stats["mean_ms"] = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
        
        // Calculate median
        std::vector<double> sortedDurations = durations;
        std::sort(sortedDurations.begin(), sortedDurations.end());
        
        if (sortedDurations.size() % 2 == 0) {
            stats["median_ms"] = (sortedDurations[sortedDurations.size() / 2 - 1] + 
                                sortedDurations[sortedDurations.size() / 2]) / 2.0;
        } else {
            stats["median_ms"] = sortedDurations[sortedDurations.size() / 2];
        }
        
        // Calculate standard deviation
        double variance = 0.0;
        for (double d : durations) {
            variance += (d - stats["mean_ms"]) * (d - stats["mean_ms"]);
        }
        variance /= durations.size();
        stats["stddev_ms"] = std::sqrt(variance);
        
        // Calculate coefficient of variation (relative standard deviation)
        stats["cv_percent"] = (stats["stddev_ms"] / stats["mean_ms"]) * 100.0;
        
        return stats;
    }
    
    // Print benchmark results
    static void printResults(const std::vector<Result>& results, bool verbose = false) {
        if (results.empty()) {
            std::cout << "No benchmark results to display." << std::endl;
            return;
        }
        
        // Print individual results if verbose
        if (verbose) {
            std::cout << "Individual benchmark results:" << std::endl;
            std::cout << "----------------------------" << std::endl;
            
            for (const auto& result : results) {
                std::cout << std::left << std::setw(40) << result.name << ": "
                          << std::fixed << std::setprecision(3) << result.milliseconds() << " ms";
                
                if (result.value) {
                    std::cout << " (value: " << *result.value << ")";
                }
                
                // Print additional metrics if present
                if (!result.metrics.empty()) {
                    for (const auto& [key, value] : result.metrics) {
                        std::cout << ", " << key << ": " << value;
                    }
                }
                
                std::cout << std::endl;
            }
            
            std::cout << std::endl;
        }
        
        // Calculate and print statistics
        auto stats = calculateStats(results);
        
        std::cout << "Benchmark Statistics" << std::endl;
        std::cout << "-------------------" << std::endl;
        std::cout << "Name: " << results[0].name << std::endl;
        std::cout << "Iterations: " << results.size() << std::endl;
        std::cout << "Min time: " << std::fixed << std::setprecision(3) << stats["min_ms"] << " ms" << std::endl;
        std::cout << "Max time: " << std::fixed << std::setprecision(3) << stats["max_ms"] << " ms" << std::endl;
        std::cout << "Mean time: " << std::fixed << std::setprecision(3) << stats["mean_ms"] << " ms" << std::endl;
        std::cout << "Median time: " << std::fixed << std::setprecision(3) << stats["median_ms"] << " ms" << std::endl;
        std::cout << "Std dev: " << std::fixed << std::setprecision(3) << stats["stddev_ms"] << " ms" << std::endl;
        std::cout << "Coefficient of variation: " << std::fixed << std::setprecision(2) 
                  << stats["cv_percent"] << "%" << std::endl;
    }
    
    // Benchmark multiple functions and compare them
    template<typename... Funcs>
    static void compare(std::string_view title, size_t iterations, 
                       const std::vector<std::string>& names, Funcs&&... funcs) {
        // Ensure we have a name for each function
        constexpr size_t funcCount = sizeof...(Funcs);
        if (names.size() != funcCount) {
            std::cerr << "Error: Number of names (" << names.size() 
                     << ") doesn't match number of functions (" << funcCount << ")" << std::endl;
            return;
        }
        
        // Run each benchmark
        std::vector<std::vector<Result>> allResults;
        runEach(allResults, iterations, names, std::forward<Funcs>(funcs)...);
        
        // Print comparison
        std::cout << title << " - Comparison" << std::endl;
        std::cout << std::string(title.length() + 14, '=') << std::endl;
        
        // Calculate stats for each function
        std::vector<std::unordered_map<std::string, double>> allStats;
        for (const auto& results : allResults) {
            allStats.push_back(calculateStats(results));
        }
        
        // Print mean times
        std::cout << std::left << std::setw(20) << "Function" 
                  << std::setw(15) << "Mean (ms)" 
                  << std::setw(15) << "Median (ms)"
                  << std::setw(15) << "StdDev (ms)"
                  << std::setw(15) << "Rel. Perf." << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        // Find the fastest function (lowest mean time)
        double fastestMean = std::numeric_limits<double>::max();
        for (const auto& stats : allStats) {
            fastestMean = std::min(fastestMean, stats.at("mean_ms"));
        }
        
        // Print results for each function
        for (size_t i = 0; i < allStats.size(); ++i) {
            const auto& stats = allStats[i];
            double relativePerf = stats.at("mean_ms") / fastestMean;
            
            std::cout << std::left << std::setw(20) << names[i]
                      << std::fixed << std::setprecision(3) 
                      << std::setw(15) << stats.at("mean_ms")
                      << std::setw(15) << stats.at("median_ms")
                      << std::setw(15) << stats.at("stddev_ms")
                      << std::setw(15) << std::fixed << std::setprecision(2) << relativePerf << "x" 
                      << std::endl;
        }
        
        std::cout << std::endl;
    }

private:
    // Helper to recursively run each function
    template<typename Func, typename... Rest>
    static void runEach(std::vector<std::vector<Result>>& allResults, size_t iterations,
                       const std::vector<std::string>& names, Func&& func, Rest&&... rest) {
        // Run the current function
        std::string name = names[allResults.size()];
        auto results = runMultiple(name, iterations, true, std::forward<Func>(func));
        allResults.push_back(results);
        
        // Run the rest of the functions
        if constexpr (sizeof...(Rest) > 0) {
            runEach(allResults, iterations, names, std::forward<Rest>(rest)...);
        }
    }
};

// Convenience macro for simple benchmarking
#define BENCHMARK(name, func, ...) \
    cachesim::Benchmark::run(name, func, ##__VA_ARGS__)

// Convenience macro for multiple iterations
#define BENCHMARK_MULTIPLE(name, iterations, func, ...) \
    cachesim::Benchmark::runMultiple(name, iterations, true, func, ##__VA_ARGS__)

} // namespace cachesim
