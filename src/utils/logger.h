#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <optional>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <memory>

namespace cachesim {

// Log level enum with string conversion
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

// Modern C++17 inline constexpr array for log level names
inline constexpr std::array<std::string_view, 5> LogLevelNames = {
    "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

// Forward declaration for friendship
class NamedLogger;

// Logger class for the cache simulator
class Logger {
public:
    // Singleton instance getter
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    // Delete copy and move constructors
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
    
    // Set log file
    void setLogFile(const std::filesystem::path& path) {
        std::lock_guard<std::mutex> lock(mutex);
        if (logFile.is_open()) {
            logFile.close();
        }
        
        logFile.open(path, std::ios::app);
        if (!logFile) {
            std::cerr << "Failed to open log file: " << path << std::endl;
        } else {
            logFilePath = path;
        }
    }
    
    // Set minimum log level
    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex);
        minLevel = level;
    }
    
    // Enable/disable console output
    void setConsoleOutput(bool enable) {
        std::lock_guard<std::mutex> lock(mutex);
        consoleOutput = enable;
    }
    
    // Create a new named logger
    [[nodiscard]] std::shared_ptr<Logger> createNamedLogger(std::string_view name) {
        std::lock_guard<std::mutex> lock(mutex);
        
        // Create a new Logger using the protected factory method
        auto logger = create();
        logger->loggerName = name;
        logger->minLevel = minLevel;
        logger->consoleOutput = consoleOutput;
        
        // Don't copy the stream, just open the same file if needed
        if (!logFilePath.empty()) {
            logger->logFilePath = logFilePath;
            logger->logFile.open(logFilePath, std::ios::app);
        }
        
        namedLoggers[std::string(name)] = logger;
        return logger;
    }
    
    // Get a named logger
    [[nodiscard]] std::shared_ptr<Logger> getNamedLogger(std::string_view name) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = namedLoggers.find(std::string(name));
        if (it != namedLoggers.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Log methods for different levels
    template<typename... Args>
    void debug(std::string_view format, Args&&... args) {
        log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void info(std::string_view format, Args&&... args) {
        log(LogLevel::Info, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void warning(std::string_view format, Args&&... args) {
        log(LogLevel::Warning, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void error(std::string_view format, Args&&... args) {
        log(LogLevel::Error, format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    void fatal(std::string_view format, Args&&... args) {
        log(LogLevel::Fatal, format, std::forward<Args>(args)...);
    }
    
    // Templated log method
    template<typename... Args>
    void log(LogLevel level, std::string_view format, Args&&... args) {
        // Skip if below minimum level
        if (level < minLevel) {
            return;
        }
        
        // Format the message using C++17 features
        std::string message = formatString(format, std::forward<Args>(args)...);
        
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto timePoint = std::chrono::system_clock::to_time_t(now);
        
        // Format timestamp
        std::ostringstream timestamp;
        timestamp << std::put_time(std::localtime(&timePoint), "%Y-%m-%d %H:%M:%S");
        
        // Format log entry
        std::ostringstream logEntry;
        logEntry << "[" << timestamp.str() << "] "
                 << "[" << LogLevelNames[static_cast<int>(level)] << "] ";
                 
        // Add logger name if present
        if (!loggerName.empty()) {
            logEntry << "[" << loggerName << "] ";
        }
        
        logEntry << message;
        
        // Lock to prevent interleaved output
        std::lock_guard<std::mutex> lock(mutex);
        
        // Write to file if enabled
        if (logFile.is_open()) {
            logFile << logEntry.str() << std::endl;
            logFile.flush();
        }
        
        // Write to console if enabled
        if (consoleOutput) {
            // Choose output stream based on level
            auto& stream = (level >= LogLevel::Warning) ? std::cerr : std::cout;
            stream << logEntry.str() << std::endl;
        }
    }
    
    // Close the log file
    void closeLogFile() {
        std::lock_guard<std::mutex> lock(mutex);
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    // Get current log file path
    [[nodiscard]] std::filesystem::path getLogFilePath() const {
        return logFilePath;
    }
    
    // Destructor
    ~Logger() {
        closeLogFile();
    }

protected:
    // Protected constructor for singleton
    Logger() 
        : minLevel(LogLevel::Info), 
          consoleOutput(true) {}
    
    // Protected factory method for creating new Logger instances
    static std::shared_ptr<Logger> create() {
        return std::shared_ptr<Logger>(new Logger());
    }
    
private:
    // String formatting helper using variadic templates
    template<typename T>
    static void formatHelper(std::ostringstream& oss, std::string_view format, size_t pos, T&& value) {
        // Find the next {} placeholder
        size_t placeholderPos = format.find("{}", pos);
        
        if (placeholderPos != std::string_view::npos) {
            // Append everything before the placeholder
            oss << format.substr(pos, placeholderPos - pos);
            // Append the value
            oss << std::forward<T>(value);
            // Append the rest of the format string
            oss << format.substr(placeholderPos + 2);
        } else {
            // No placeholder found, just append the rest
            oss << format.substr(pos);
        }
    }
    
    template<typename T, typename... Args>
    static void formatHelper(std::ostringstream& oss, std::string_view format, size_t pos, T&& value, Args&&... args) {
        // Find the next {} placeholder
        size_t placeholderPos = format.find("{}", pos);
        
        if (placeholderPos != std::string_view::npos) {
            // Append everything before the placeholder
            oss << format.substr(pos, placeholderPos - pos);
            // Append the value
            oss << std::forward<T>(value);
            // Continue with the rest
            formatHelper(oss, format, placeholderPos + 2, std::forward<Args>(args)...);
        } else {
            // No more placeholders, just append the rest
            oss << format.substr(pos);
        }
    }
    
    // Format string with placeholder substitution
    template<typename... Args>
    static std::string formatString(std::string_view format, Args&&... args) {
        if constexpr (sizeof...(args) == 0) {
            return std::string(format);
        } else {
            std::ostringstream oss;
            formatHelper(oss, format, 0, std::forward<Args>(args)...);
            return oss.str();
        }
    }
    
    std::string loggerName;               // Name for this logger instance
    LogLevel minLevel;                    // Minimum log level to output
    bool consoleOutput;                   // Whether to output to console
    std::ofstream logFile;                // Log file stream
    std::filesystem::path logFilePath;    // Path to log file
    std::mutex mutex;                     // Mutex for thread safety
    
    // Map of named loggers
    std::unordered_map<std::string, std::shared_ptr<Logger>> namedLoggers;
    
    // Buffered logging support (v1.1.0)
    static constexpr size_t bufferSize = 4096;
    std::string logBuffer;
    bool bufferingEnabled = false;
    
    // Flush buffer to file
    void flushBuffer() {
        if (!logBuffer.empty() && logFile.is_open()) {
            logFile << logBuffer;
            logFile.flush();
            logBuffer.clear();
        }
    }
    
public:
    // Enable/disable buffering (v1.1.0)
    void setBuffering(bool enable) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!enable && bufferingEnabled) {
            flushBuffer();
        }
        bufferingEnabled = enable;
    }
    
    // Manual flush (v1.1.0)
    void flush() {
        std::lock_guard<std::mutex> lock(mutex);
        flushBuffer();
    }
};

// Convenience macro for getting the logger
#define LOG Logger::getInstance()

// Convenience macros for logging
#define LOG_DEBUG(...)    Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...)     Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARNING(...)  Logger::getInstance().warning(__VA_ARGS__)
#define LOG_ERROR(...)    Logger::getInstance().error(__VA_ARGS__)
#define LOG_FATAL(...)    Logger::getInstance().fatal(__VA_ARGS__)

} // namespace cachesim