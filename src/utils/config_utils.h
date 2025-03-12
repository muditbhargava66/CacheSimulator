#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <any>
#include <variant>

#include "../core/memory_hierarchy.h"

namespace cachesim {

/**
 * Configuration utility class for loading and saving cache simulator configurations.
 * Uses C++17 features like std::filesystem, std::optional, and std::variant.
 */
class ConfigManager {
public:
    // Configuration format types
    enum class ConfigFormat {
        JSON,
        INI,
        CommandLine
    };
    
    // Structure to represent a simulator configuration
    struct SimulatorConfig {
        std::string name;
        MemoryHierarchyConfig hierarchyConfig;
        std::unordered_map<std::string, std::any> extraOptions;
    };
    
    // Constructor
    explicit ConfigManager(ConfigFormat format = ConfigFormat::JSON);
    
    // Load configuration from file
    [[nodiscard]] std::optional<SimulatorConfig> loadConfig(const std::filesystem::path& configPath);
    
    // Save configuration to file
    bool saveConfig(const std::filesystem::path& configPath, const SimulatorConfig& config);
    
    // Create config from command line arguments
    [[nodiscard]] std::optional<SimulatorConfig> createFromCommandLine(int argc, char* argv[]);
    
    // Generate a default configuration
    [[nodiscard]] static SimulatorConfig getDefaultConfig();
    
    // Compare two configurations
    [[nodiscard]] static std::vector<std::string> compareConfigs(const SimulatorConfig& config1, 
                                                                const SimulatorConfig& config2);
    
    // Generate batch configurations for parameter sweeps
    [[nodiscard]] static std::vector<SimulatorConfig> generateParameterSweep(
        const SimulatorConfig& baseConfig,
        const std::string& paramName,
        const std::vector<std::variant<int, bool, double, std::string>>& values
    );
    
    // Parse simple config string (for quick setups)
    [[nodiscard]] static std::optional<SimulatorConfig> parseConfigString(std::string_view configStr);
    
    // Configuration validation
    [[nodiscard]] static bool validateConfig(const SimulatorConfig& config, std::string& errorMessage);

private:
    ConfigFormat format;
    
    // Internal parsing methods
    std::optional<SimulatorConfig> parseJsonConfig(const std::filesystem::path& configPath);
    std::optional<SimulatorConfig> parseIniConfig(const std::filesystem::path& configPath);
    
    // Internal serialization methods
    bool writeJsonConfig(const std::filesystem::path& configPath, const SimulatorConfig& config);
    bool writeIniConfig(const std::filesystem::path& configPath, const SimulatorConfig& config);
    
    // Helper methods for validation
    static bool isValidSize(int size) { return size > 0 && (size & (size - 1)) == 0; }
    static bool isValidAssociativity(int assoc) { return assoc > 0; }
};

} // namespace cachesim