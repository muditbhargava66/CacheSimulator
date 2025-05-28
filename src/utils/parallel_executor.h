/**
 * @file parallel_executor.h
 * @brief Parallel execution framework for cache simulation
 * @author Mudit Bhargava
 * @date 2025-05-29
 * @version 1.2.0
 *
 * This file implements parallel processing capabilities for running
 * cache simulations on large trace files using multiple threads.
 */

#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <functional>
#include <memory>
#include <chrono>
#include <optional>

namespace cachesim {

/**
 * @class ThreadPool
 * @brief Simple thread pool for parallel task execution
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency())
        : stop_(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                for (;;) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        condition_.wait(lock, [this] { 
                            return stop_ || !tasks_.empty(); 
                        });
                        
                        if (stop_ && tasks_.empty()) {
                            return;
                        }
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        
        condition_.notify_all();
        
        for (std::thread& worker : workers_) {
            worker.join();
        }
    }
    
    /**
     * @brief Enqueue a task for execution
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return Future for the result
     */
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            
            tasks_.emplace([task]() { (*task)(); });
        }
        
        condition_.notify_one();
        return res;
    }
    
    /**
     * @brief Get the number of worker threads
     */
    size_t getNumThreads() const { return workers_.size(); }
    
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    bool stop_;
};

/**
 * @class ParallelTraceProcessor
 * @brief Process memory traces in parallel chunks
 */
template<typename CacheSimulator>
class ParallelTraceProcessor {
public:
    struct ProcessingResult {
        uint64_t totalAccesses = 0;
        uint64_t hits = 0;
        uint64_t misses = 0;
        uint64_t prefetches = 0;
        std::chrono::nanoseconds processingTime;
    };
    
    /**
     * @brief Constructor
     * @param numThreads Number of threads to use (0 = hardware concurrency)
     */
    explicit ParallelTraceProcessor(size_t numThreads = 0)
        : threadPool_(numThreads == 0 ? std::thread::hardware_concurrency() : numThreads) {}
    
    /**
     * @brief Process a trace file in parallel
     * @param traceFile Path to the trace file
     * @param cacheConfig Configuration for cache simulators
     * @param chunkSize Number of accesses per chunk
     * @return Combined processing results
     */
    ProcessingResult processTrace(
        const std::filesystem::path& traceFile,
        const typename CacheSimulator::Config& cacheConfig,
        size_t chunkSize = 10000) {
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Read trace into chunks
        std::vector<std::vector<uint32_t>> chunks = partitionTrace(traceFile, chunkSize);
        
        // Process chunks in parallel
        std::vector<std::future<ProcessingResult>> futures;
        
        for (const auto& chunk : chunks) {
            futures.push_back(
                threadPool_.enqueue([this, chunk, cacheConfig]() {
                    return processChunk(chunk, cacheConfig);
                })
            );
        }
        
        // Collect results
        ProcessingResult combined;
        for (auto& future : futures) {
            auto result = future.get();
            combined.totalAccesses += result.totalAccesses;
            combined.hits += result.hits;
            combined.misses += result.misses;
            combined.prefetches += result.prefetches;
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        combined.processingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
            endTime - startTime);
        
        return combined;
    }
    
    /**
     * @brief Process multiple configurations in parallel
     * @param traceFile Path to the trace file
     * @param configs Vector of cache configurations to test
     * @return Results for each configuration
     */
    std::vector<std::pair<typename CacheSimulator::Config, ProcessingResult>>
    processMultipleConfigs(
        const std::filesystem::path& traceFile,
        const std::vector<typename CacheSimulator::Config>& configs) {
        
        std::vector<std::future<std::pair<typename CacheSimulator::Config, ProcessingResult>>> futures;
        
        // Launch parallel simulations for each config
        for (const auto& config : configs) {
            futures.push_back(
                threadPool_.enqueue([this, traceFile, config]() {
                    auto result = processTrace(traceFile, config);
                    return std::make_pair(config, result);
                })
            );
        }
        
        // Collect results
        std::vector<std::pair<typename CacheSimulator::Config, ProcessingResult>> results;
        for (auto& future : futures) {
            results.push_back(future.get());
        }
        
        return results;
    }
    
private:
    /**
     * @brief Partition trace file into chunks
     */
    std::vector<std::vector<uint32_t>> partitionTrace(
        const std::filesystem::path& traceFile,
        size_t chunkSize) {
        
        std::vector<std::vector<uint32_t>> chunks;
        std::ifstream file(traceFile);
        
        if (!file) {
            throw std::runtime_error("Cannot open trace file: " + traceFile.string());
        }
        
        std::vector<uint32_t> currentChunk;
        currentChunk.reserve(chunkSize);
        
        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Parse address (simple format: "R/W address")
            char op;
            uint32_t address;
            if (std::sscanf(line.c_str(), "%c %x", &op, &address) == 2) {
                currentChunk.push_back(address);
                
                if (currentChunk.size() >= chunkSize) {
                    chunks.push_back(std::move(currentChunk));
                    currentChunk = std::vector<uint32_t>();
                    currentChunk.reserve(chunkSize);
                }
            }
        }
        
        // Add remaining addresses
        if (!currentChunk.empty()) {
            chunks.push_back(std::move(currentChunk));
        }
        
        return chunks;
    }
    
    /**
     * @brief Process a single chunk of addresses
     */
    ProcessingResult processChunk(
        const std::vector<uint32_t>& addresses,
        const typename CacheSimulator::Config& config) {
        
        ProcessingResult result;
        
        // Create local cache simulator
        CacheSimulator simulator(config);
        
        // Process addresses
        for (uint32_t address : addresses) {
            bool hit = simulator.access(address);
            result.totalAccesses++;
            
            if (hit) {
                result.hits++;
            } else {
                result.misses++;
            }
        }
        
        // Get prefetch statistics if available
        auto stats = simulator.getStatistics();
        result.prefetches = stats.prefetches;
        
        return result;
    }
    
    ThreadPool threadPool_;
};

/**
 * @class ParallelBenchmark
 * @brief Run benchmarks in parallel for multiple configurations
 */
class ParallelBenchmark {
public:
    struct BenchmarkConfig {
        std::string name;
        std::function<void()> setupFunc;
        std::function<double()> benchmarkFunc;
        std::function<void()> teardownFunc;
        int iterations = 100;
    };
    
    struct BenchmarkResult {
        std::string name;
        std::vector<double> times;  // Milliseconds
        double mean;
        double median;
        double stddev;
        double min;
        double max;
    };
    
    /**
     * @brief Run benchmarks in parallel
     * @param configs Vector of benchmark configurations
     * @param numThreads Number of threads to use
     * @return Results for each benchmark
     */
    static std::vector<BenchmarkResult> runParallel(
        const std::vector<BenchmarkConfig>& configs,
        size_t numThreads = 0) {
        
        ThreadPool pool(numThreads == 0 ? std::thread::hardware_concurrency() : numThreads);
        std::vector<std::future<BenchmarkResult>> futures;
        
        // Launch benchmarks
        for (const auto& config : configs) {
            futures.push_back(
                pool.enqueue([config]() {
                    return runSingleBenchmark(config);
                })
            );
        }
        
        // Collect results
        std::vector<BenchmarkResult> results;
        for (auto& future : futures) {
            results.push_back(future.get());
        }
        
        return results;
    }
    
private:
    static BenchmarkResult runSingleBenchmark(const BenchmarkConfig& config) {
        BenchmarkResult result;
        result.name = config.name;
        result.times.reserve(config.iterations);
        
        // Setup
        if (config.setupFunc) {
            config.setupFunc();
        }
        
        // Run iterations
        for (int i = 0; i < config.iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            config.benchmarkFunc();
            auto end = std::chrono::high_resolution_clock::now();
            
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            result.times.push_back(ms);
        }
        
        // Teardown
        if (config.teardownFunc) {
            config.teardownFunc();
        }
        
        // Calculate statistics
        result.mean = std::accumulate(result.times.begin(), result.times.end(), 0.0) / result.times.size();
        
        // Median
        std::vector<double> sorted = result.times;
        std::sort(sorted.begin(), sorted.end());
        if (sorted.size() % 2 == 0) {
            result.median = (sorted[sorted.size() / 2 - 1] + sorted[sorted.size() / 2]) / 2.0;
        } else {
            result.median = sorted[sorted.size() / 2];
        }
        
        // Min/Max
        result.min = *std::min_element(result.times.begin(), result.times.end());
        result.max = *std::max_element(result.times.begin(), result.times.end());
        
        // Standard deviation
        double variance = 0.0;
        for (double t : result.times) {
            variance += (t - result.mean) * (t - result.mean);
        }
        result.stddev = std::sqrt(variance / result.times.size());
        
        return result;
    }
};

} // namespace cachesim
