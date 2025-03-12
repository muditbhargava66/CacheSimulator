#include "config_utils.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

// As this is just an example, we'll implement a simple version
// In a real project, you would use a proper JSON or INI parser library

namespace cachesim {

ConfigManager::ConfigManager(ConfigFormat format)
    : format(format) {
}

std::optional<ConfigManager::SimulatorConfig> ConfigManager::loadConfig(const std::filesystem::path& configPath) {
    if (!std::filesystem::exists(configPath)) {
        std::cerr << "Error: Configuration file " << configPath << " does not exist." << std::endl;
        return std::nullopt;
    }
    
    try {
        switch (format) {
            case ConfigFormat::JSON:
                return parseJsonConfig(configPath);
            case ConfigFormat::INI:
                return parseIniConfig(configPath);
            case ConfigFormat::CommandLine:
                std::cerr << "Error: Cannot load command line format from file." << std::endl;
                return std::nullopt;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing configuration file: " << e.what() << std::endl;
        return std::nullopt;
    }
    
    return std::nullopt;
}

bool ConfigManager::saveConfig(const std::filesystem::path& configPath, const SimulatorConfig& config) {
    try {
        switch (format) {
            case ConfigFormat::JSON:
                return writeJsonConfig(configPath, config);
            case ConfigFormat::INI:
                return writeIniConfig(configPath, config);
            case ConfigFormat::CommandLine:
                std::cerr << "Error: Cannot save to command line format." << std::endl;
                return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving configuration file: " << e.what() << std::endl;
        return false;
    }
    
    return false;
}

std::optional<ConfigManager::SimulatorConfig> ConfigManager::createFromCommandLine(int argc, char* argv[]) {
    if (argc < 8) {
        std::cerr << "Not enough arguments for configuration." << std::endl;
        return std::nullopt;
    }
    
    try {
        SimulatorConfig config;
        config.name = "Command Line Configuration";
        
        // Parse command line arguments
        int blockSize = std::stoi(argv[1]);
        int l1Size = std::stoi(argv[2]);
        int l1Assoc = std::stoi(argv[3]);
        int l2Size = std::stoi(argv[4]);
        int l2Assoc = std::stoi(argv[5]);
        bool prefetchEnabled = std::stoi(argv[6]) > 0;
        int prefetchDistance = std::stoi(argv[7]);
        
        // Validate arguments
        std::string errorMsg;
        if (!isValidSize(blockSize)) {
            std::cerr << "Invalid block size: must be a power of 2" << std::endl;
            return std::nullopt;
        }
        
        if (!isValidSize(l1Size)) {
            std::cerr << "Invalid L1 size: must be a power of 2" << std::endl;
            return std::nullopt;
        }
        
        if (!isValidAssociativity(l1Assoc)) {
            std::cerr << "Invalid L1 associativity: must be positive" << std::endl;
            return std::nullopt;
        }
        
        if (l2Size < 0) {
            std::cerr << "Invalid L2 size: cannot be negative" << std::endl;
            return std::nullopt;
        }
        
        if (l2Size > 0 && !isValidAssociativity(l2Assoc)) {
            std::cerr << "Invalid L2 associativity: must be positive when L2 is enabled" << std::endl;
            return std::nullopt;
        }
        
        if (prefetchEnabled && prefetchDistance <= 0) {
            std::cerr << "Invalid prefetch distance: must be positive when prefetching is enabled" << std::endl;
            return std::nullopt;
        }
        
        // Create L1 config
        CacheConfig l1Config{l1Size, l1Assoc, blockSize, prefetchEnabled, prefetchDistance};
        
        // Create L2 config if needed
        std::optional<CacheConfig> l2Config;
        if (l2Size > 0) {
            l2Config = CacheConfig{l2Size, l2Assoc, blockSize, prefetchEnabled, prefetchDistance};
        }
        
        // Set up hierarchy config
        config.hierarchyConfig.l1Config = l1Config;
        config.hierarchyConfig.l2Config = l2Config;
        config.hierarchyConfig.useStridePrediction = prefetchEnabled;
        config.hierarchyConfig.useAdaptivePrefetching = prefetchEnabled;
        
        // Add any additional options from remaining arguments
        for (int i = 8; i < argc; i++) {
            std::string arg = argv[i];
            if (arg.substr(0, 2) == "--") {
                std::string optName = arg.substr(2);
                std::string optValue = (i + 1 < argc) ? argv[++i] : "true";
                config.extraOptions[optName] = optValue;
            }
        }
        
        return config;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing command line arguments: " << e.what() << std::endl;
        return std::nullopt;
    }
}

ConfigManager::SimulatorConfig ConfigManager::getDefaultConfig() {
    SimulatorConfig config;
    config.name = "Default Configuration";
    
    // Default cache parameters
    int blockSize = 64;
    int l1Size = 32 * 1024;     // 32 KB
    int l1Assoc = 4;
    int l2Size = 256 * 1024;    // 256 KB
    int l2Assoc = 8;
    bool prefetchEnabled = true;
    int prefetchDistance = 4;
    
    // Create L1 config
    CacheConfig l1Config{l1Size, l1Assoc, blockSize, prefetchEnabled, prefetchDistance};
    
    // Create L2 config
    CacheConfig l2Config{l2Size, l2Assoc, blockSize, prefetchEnabled, prefetchDistance};
    
    // Set up hierarchy config
    config.hierarchyConfig.l1Config = l1Config;
    config.hierarchyConfig.l2Config = l2Config;
    config.hierarchyConfig.useStridePrediction = true;
    config.hierarchyConfig.useAdaptivePrefetching = true;
    config.hierarchyConfig.strideTableSize = 1024;
    
    return config;
}

std::vector<std::string> ConfigManager::compareConfigs(const SimulatorConfig& config1, const SimulatorConfig& config2) {
    std::vector<std::string> differences;
    
    // Compare L1 cache parameters
    if (config1.hierarchyConfig.l1Config.size != config2.hierarchyConfig.l1Config.size) {
        differences.push_back("L1 size: " + std::to_string(config1.hierarchyConfig.l1Config.size) + 
                              " vs " + std::to_string(config2.hierarchyConfig.l1Config.size));
    }
    
    if (config1.hierarchyConfig.l1Config.associativity != config2.hierarchyConfig.l1Config.associativity) {
        differences.push_back("L1 associativity: " + std::to_string(config1.hierarchyConfig.l1Config.associativity) + 
                              " vs " + std::to_string(config2.hierarchyConfig.l1Config.associativity));
    }
    
    if (config1.hierarchyConfig.l1Config.blockSize != config2.hierarchyConfig.l1Config.blockSize) {
        differences.push_back("Block size: " + std::to_string(config1.hierarchyConfig.l1Config.blockSize) + 
                              " vs " + std::to_string(config2.hierarchyConfig.l1Config.blockSize));
    }
    
    if (config1.hierarchyConfig.l1Config.prefetchEnabled != config2.hierarchyConfig.l1Config.prefetchEnabled) {
        differences.push_back("Prefetching: " + std::string(config1.hierarchyConfig.l1Config.prefetchEnabled ? "enabled" : "disabled") + 
                              " vs " + std::string(config2.hierarchyConfig.l1Config.prefetchEnabled ? "enabled" : "disabled"));
    }
    
    if (config1.hierarchyConfig.l1Config.prefetchDistance != config2.hierarchyConfig.l1Config.prefetchDistance) {
        differences.push_back("Prefetch distance: " + std::to_string(config1.hierarchyConfig.l1Config.prefetchDistance) + 
                              " vs " + std::to_string(config2.hierarchyConfig.l1Config.prefetchDistance));
    }
    
    // Compare L2 cache parameters (if present)
    bool config1HasL2 = config1.hierarchyConfig.l2Config.has_value();
    bool config2HasL2 = config2.hierarchyConfig.l2Config.has_value();
    
    if (config1HasL2 != config2HasL2) {
        differences.push_back("L2 cache: " + std::string(config1HasL2 ? "present" : "absent") + 
                              " vs " + std::string(config2HasL2 ? "present" : "absent"));
    } else if (config1HasL2 && config2HasL2) {
        // Both have L2, compare parameters
        if (config1.hierarchyConfig.l2Config->size != config2.hierarchyConfig.l2Config->size) {
            differences.push_back("L2 size: " + std::to_string(config1.hierarchyConfig.l2Config->size) + 
                                  " vs " + std::to_string(config2.hierarchyConfig.l2Config->size));
        }
        
        if (config1.hierarchyConfig.l2Config->associativity != config2.hierarchyConfig.l2Config->associativity) {
            differences.push_back("L2 associativity: " + std::to_string(config1.hierarchyConfig.l2Config->associativity) + 
                                  " vs " + std::to_string(config2.hierarchyConfig.l2Config->associativity));
        }
    }
    
    // Compare prefetching strategies
    if (config1.hierarchyConfig.useStridePrediction != config2.hierarchyConfig.useStridePrediction) {
        differences.push_back("Stride prediction: " + std::string(config1.hierarchyConfig.useStridePrediction ? "enabled" : "disabled") + 
                              " vs " + std::string(config2.hierarchyConfig.useStridePrediction ? "enabled" : "disabled"));
    }
    
    if (config1.hierarchyConfig.useAdaptivePrefetching != config2.hierarchyConfig.useAdaptivePrefetching) {
        differences.push_back("Adaptive prefetching: " + std::string(config1.hierarchyConfig.useAdaptivePrefetching ? "enabled" : "disabled") + 
                              " vs " + std::string(config2.hierarchyConfig.useAdaptivePrefetching ? "enabled" : "disabled"));
    }
    
    return differences;
}

std::vector<ConfigManager::SimulatorConfig> ConfigManager::generateParameterSweep(
    const SimulatorConfig& baseConfig,
    const std::string& paramName,
    const std::vector<std::variant<int, bool, double, std::string>>& values) {
    
    std::vector<SimulatorConfig> configs;
    
    for (const auto& value : values) {
        SimulatorConfig newConfig = baseConfig;
        newConfig.name = baseConfig.name + " (" + paramName + "=";
        
        // Apply the parameter value based on parameter name
        if (paramName == "l1_size") {
            int sizeValue = std::get<int>(value);
            newConfig.hierarchyConfig.l1Config.size = sizeValue;
            newConfig.name += std::to_string(sizeValue);
        } else if (paramName == "l1_assoc") {
            int assocValue = std::get<int>(value);
            newConfig.hierarchyConfig.l1Config.associativity = assocValue;
            newConfig.name += std::to_string(assocValue);
        } else if (paramName == "l2_size") {
            int sizeValue = std::get<int>(value);
            if (sizeValue > 0) {
                if (newConfig.hierarchyConfig.l2Config) {
                    newConfig.hierarchyConfig.l2Config->size = sizeValue;
                } else {
                    // Create L2 config with default parameters
                    CacheConfig l2Config{sizeValue, 8, newConfig.hierarchyConfig.l1Config.blockSize, 
                                       newConfig.hierarchyConfig.l1Config.prefetchEnabled,
                                       newConfig.hierarchyConfig.l1Config.prefetchDistance};
                    newConfig.hierarchyConfig.l2Config = l2Config;
                }
            } else {
                // Disable L2
                newConfig.hierarchyConfig.l2Config = std::nullopt;
            }
            newConfig.name += std::to_string(sizeValue);
        } else if (paramName == "l2_assoc") {
            int assocValue = std::get<int>(value);
            if (newConfig.hierarchyConfig.l2Config) {
                newConfig.hierarchyConfig.l2Config->associativity = assocValue;
            }
            newConfig.name += std::to_string(assocValue);
        } else if (paramName == "block_size") {
            int blockValue = std::get<int>(value);
            newConfig.hierarchyConfig.l1Config.blockSize = blockValue;
            if (newConfig.hierarchyConfig.l2Config) {
                newConfig.hierarchyConfig.l2Config->blockSize = blockValue;
            }
            newConfig.name += std::to_string(blockValue);
        } else if (paramName == "prefetch") {
            bool prefValue = std::get<bool>(value);
            newConfig.hierarchyConfig.l1Config.prefetchEnabled = prefValue;
            if (newConfig.hierarchyConfig.l2Config) {
                newConfig.hierarchyConfig.l2Config->prefetchEnabled = prefValue;
            }
            newConfig.hierarchyConfig.useStridePrediction = prefValue;
            newConfig.hierarchyConfig.useAdaptivePrefetching = prefValue;
            newConfig.name += prefValue ? "true" : "false";
        } else if (paramName == "prefetch_distance") {
            int distValue = std::get<int>(value);
            newConfig.hierarchyConfig.l1Config.prefetchDistance = distValue;
            if (newConfig.hierarchyConfig.l2Config) {
                newConfig.hierarchyConfig.l2Config->prefetchDistance = distValue;
            }
            newConfig.name += std::to_string(distValue);
        } else {
            // Store in extra options
            std::visit([&](const auto& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, int>) {
                    newConfig.extraOptions[paramName] = val;
                    newConfig.name += std::to_string(val);
                } else if constexpr (std::is_same_v<T, bool>) {
                    newConfig.extraOptions[paramName] = val;
                    newConfig.name += val ? "true" : "false";
                } else if constexpr (std::is_same_v<T, double>) {
                    newConfig.extraOptions[paramName] = val;
                    newConfig.name += std::to_string(val);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    newConfig.extraOptions[paramName] = val;
                    newConfig.name += val;  // For strings, simply append the string directly
                }
            }, value);
        }
        
        newConfig.name += ")";
        configs.push_back(newConfig);
    }
    
    return configs;
}

std::optional<ConfigManager::SimulatorConfig> ConfigManager::parseConfigString(std::string_view configStr) {
    // Fix: Use brace initialization to avoid vexing parse
    std::istringstream iss{std::string(configStr)};
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(iss, token, ':')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() < 6) {
        std::cerr << "Invalid config string format. Expected l1size:l1assoc:l2size:l2assoc:blocksize:prefetch[:distance]" << std::endl;
        return std::nullopt;
    }
    
    try {
        SimulatorConfig config;
        config.name = "Quick Configuration";
        
        int l1Size = std::stoi(tokens[0]);
        int l1Assoc = std::stoi(tokens[1]);
        int l2Size = std::stoi(tokens[2]);
        int l2Assoc = std::stoi(tokens[3]);
        int blockSize = std::stoi(tokens[4]);
        bool prefetchEnabled = (tokens[5] == "1" || tokens[5] == "true" || tokens[5] == "yes");
        int prefetchDistance = tokens.size() > 6 ? std::stoi(tokens[6]) : 4;
        
        // L1 config
        CacheConfig l1Config{l1Size, l1Assoc, blockSize, prefetchEnabled, prefetchDistance};
        
        // L2 config if present
        std::optional<CacheConfig> l2Config;
        if (l2Size > 0) {
            l2Config = CacheConfig{l2Size, l2Assoc, blockSize, prefetchEnabled, prefetchDistance};
        }
        
        // Set hierarchy config
        config.hierarchyConfig.l1Config = l1Config;
        config.hierarchyConfig.l2Config = l2Config;
        config.hierarchyConfig.useStridePrediction = prefetchEnabled;
        config.hierarchyConfig.useAdaptivePrefetching = prefetchEnabled;
        
        // Validate
        std::string errorMsg;
        if (!validateConfig(config, errorMsg)) {
            std::cerr << "Invalid configuration: " << errorMsg << std::endl;
            return std::nullopt;
        }
        
        return config;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config string: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool ConfigManager::validateConfig(const SimulatorConfig& config, std::string& errorMessage) {
    // Validate L1 config
    if (!isValidSize(config.hierarchyConfig.l1Config.size)) {
        errorMessage = "L1 size must be a positive power of 2";
        return false;
    }
    
    if (!isValidAssociativity(config.hierarchyConfig.l1Config.associativity)) {
        errorMessage = "L1 associativity must be positive";
        return false;
    }
    
    if (!isValidSize(config.hierarchyConfig.l1Config.blockSize)) {
        errorMessage = "Block size must be a positive power of 2";
        return false;
    }
    
    if (config.hierarchyConfig.l1Config.prefetchEnabled && config.hierarchyConfig.l1Config.prefetchDistance <= 0) {
        errorMessage = "Prefetch distance must be positive when prefetching is enabled";
        return false;
    }
    
    // Validate L2 config if present
    if (config.hierarchyConfig.l2Config) {
        if (!isValidSize(config.hierarchyConfig.l2Config->size)) {
            errorMessage = "L2 size must be a positive power of 2";
            return false;
        }
        
        if (!isValidAssociativity(config.hierarchyConfig.l2Config->associativity)) {
            errorMessage = "L2 associativity must be positive";
            return false;
        }
        
        if (config.hierarchyConfig.l2Config->blockSize != config.hierarchyConfig.l1Config.blockSize) {
            errorMessage = "L1 and L2 must have the same block size";
            return false;
        }
    }
    
    return true;
}

// Simple JSON parser implementation (in a real project, use a library)
std::optional<ConfigManager::SimulatorConfig> ConfigManager::parseJsonConfig(const std::filesystem::path& configPath) {
    // This is a simplified implementation
    // In a real project, use a proper JSON library like nlohmann/json
    
    std::ifstream file(configPath);
    if (!file) {
        return std::nullopt;
    }
    
    std::string line;
    SimulatorConfig config;
    config.name = "JSON Configuration";
    
    // Very simple JSON parser (not robust, just for example)
    while (std::getline(file, line)) {
        // Remove whitespace
        line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char c) { 
            return std::isspace(c); 
        }), line.end());
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '/') {
            continue;
        }
        
        // Skip start/end braces
        if (line == "{" || line == "}") {
            continue;
        }
        
        // Remove commas and quotes
        line.erase(std::remove(line.begin(), line.end(), ','), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\"'), line.end());
        
        // Split key:value
        auto pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Parse specific configuration parameters
            if (key == "l1_size") {
                config.hierarchyConfig.l1Config.size = std::stoi(value);
            } else if (key == "l1_assoc") {
                config.hierarchyConfig.l1Config.associativity = std::stoi(value);
            } else if (key == "l2_size") {
                int l2Size = std::stoi(value);
                if (l2Size > 0) {
                    // Initialize L2 if not already present
                    if (!config.hierarchyConfig.l2Config) {
                        config.hierarchyConfig.l2Config = CacheConfig{};
                    }
                    config.hierarchyConfig.l2Config->size = l2Size;
                }
            } else if (key == "l2_assoc") {
                int l2Assoc = std::stoi(value);
                if (!config.hierarchyConfig.l2Config) {
                    config.hierarchyConfig.l2Config = CacheConfig{};
                }
                config.hierarchyConfig.l2Config->associativity = l2Assoc;
            } else if (key == "block_size") {
                int blockSize = std::stoi(value);
                config.hierarchyConfig.l1Config.blockSize = blockSize;
                if (config.hierarchyConfig.l2Config) {
                    config.hierarchyConfig.l2Config->blockSize = blockSize;
                }
            } else if (key == "prefetch_enabled") {
                bool prefEnabled = (value == "true");
                config.hierarchyConfig.l1Config.prefetchEnabled = prefEnabled;
                if (config.hierarchyConfig.l2Config) {
                    config.hierarchyConfig.l2Config->prefetchEnabled = prefEnabled;
                }
                config.hierarchyConfig.useStridePrediction = prefEnabled;
                config.hierarchyConfig.useAdaptivePrefetching = prefEnabled;
            } else if (key == "prefetch_distance") {
                int prefDist = std::stoi(value);
                config.hierarchyConfig.l1Config.prefetchDistance = prefDist;
                if (config.hierarchyConfig.l2Config) {
                    config.hierarchyConfig.l2Config->prefetchDistance = prefDist;
                }
            } else if (key == "name") {
                config.name = value;
            } else {
                // Store any other parameters in extraOptions
                config.extraOptions[key] = value;
            }
        }
    }
    
    // Ensure L2 has same block size as L1 if present
    if (config.hierarchyConfig.l2Config) {
        config.hierarchyConfig.l2Config->blockSize = config.hierarchyConfig.l1Config.blockSize;
    }
    
    return config;
}

bool ConfigManager::writeJsonConfig(const std::filesystem::path& configPath, const SimulatorConfig& config) {
    // This is a simplified implementation
    // In a real project, use a proper JSON library
    
    std::ofstream file(configPath);
    if (!file) {
        return false;
    }
    
    file << "{\n";
    file << "  \"name\": \"" << config.name << "\",\n";
    file << "  \"l1_size\": " << config.hierarchyConfig.l1Config.size << ",\n";
    file << "  \"l1_assoc\": " << config.hierarchyConfig.l1Config.associativity << ",\n";
    file << "  \"block_size\": " << config.hierarchyConfig.l1Config.blockSize << ",\n";
    file << "  \"prefetch_enabled\": " << (config.hierarchyConfig.l1Config.prefetchEnabled ? "true" : "false") << ",\n";
    file << "  \"prefetch_distance\": " << config.hierarchyConfig.l1Config.prefetchDistance << ",\n";
    
    if (config.hierarchyConfig.l2Config) {
        file << "  \"l2_size\": " << config.hierarchyConfig.l2Config->size << ",\n";
        file << "  \"l2_assoc\": " << config.hierarchyConfig.l2Config->associativity << ",\n";
    } else {
        file << "  \"l2_size\": 0,\n";
        file << "  \"l2_assoc\": 0,\n";
    }
    
    file << "  \"use_stride_prediction\": " << (config.hierarchyConfig.useStridePrediction ? "true" : "false") << ",\n";
    file << "  \"use_adaptive_prefetching\": " << (config.hierarchyConfig.useAdaptivePrefetching ? "true" : "false") << ",\n";
    file << "  \"stride_table_size\": " << config.hierarchyConfig.strideTableSize << ",\n";
    
    // Write extra options
    for (const auto& [key, value] : config.extraOptions) {
        try {
            if (value.type() == typeid(std::string)) {
                file << "  \"" << key << "\": \"" << std::any_cast<std::string>(value) << "\",\n";
            } else if (value.type() == typeid(int)) {
                file << "  \"" << key << "\": " << std::any_cast<int>(value) << ",\n";
            } else if (value.type() == typeid(bool)) {
                file << "  \"" << key << "\": " << (std::any_cast<bool>(value) ? "true" : "false") << ",\n";
            } else if (value.type() == typeid(double)) {
                file << "  \"" << key << "\": " << std::any_cast<double>(value) << ",\n";
            }
        } catch (const std::bad_any_cast& e) {
            // Handle potential casting errors
            std::cerr << "Error casting value for key '" << key << "': " << e.what() << std::endl;
            file << "  \"" << key << "\": \"ERROR\",\n";
        }
    }
    
    // Remove trailing comma and close JSON object
    file.seekp(-2, std::ios_base::cur);
    file << "\n}";
    
    return true;
}

std::optional<ConfigManager::SimulatorConfig> ConfigManager::parseIniConfig(const std::filesystem::path& configPath) {
    // This is a simplified implementation
    // In a real project, use a proper INI parser
    
    std::ifstream file(configPath);
    if (!file) {
        return std::nullopt;
    }
    
    SimulatorConfig config;
    config.name = "INI Configuration";
    
    std::string line;
    std::string currentSection;
    
    // Very simple INI parser
    while (std::getline(file, line)) {
        // Remove whitespace from beginning and end
        line = line.substr(line.find_first_not_of(" \t"));
        line = line.substr(0, line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        
        // Check for section headers
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }
        
        // Parse key=value pairs
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // Remove whitespace
            key = key.substr(0, key.find_last_not_of(" \t") + 1);
            value = value.substr(value.find_first_not_of(" \t"));
            
            // Parse based on section
            if (currentSection == "L1Cache") {
                if (key == "size") {
                    config.hierarchyConfig.l1Config.size = std::stoi(value);
                } else if (key == "associativity") {
                    config.hierarchyConfig.l1Config.associativity = std::stoi(value);
                } else if (key == "block_size") {
                    config.hierarchyConfig.l1Config.blockSize = std::stoi(value);
                } else if (key == "prefetch_enabled") {
                    config.hierarchyConfig.l1Config.prefetchEnabled = (value == "1" || value == "true" || value == "yes");
                } else if (key == "prefetch_distance") {
                    config.hierarchyConfig.l1Config.prefetchDistance = std::stoi(value);
                }
            } else if (currentSection == "L2Cache") {
                // Initialize L2 if not already present
                if (!config.hierarchyConfig.l2Config) {
                    config.hierarchyConfig.l2Config = CacheConfig{};
                }
                
                if (key == "size") {
                    config.hierarchyConfig.l2Config->size = std::stoi(value);
                } else if (key == "associativity") {
                    config.hierarchyConfig.l2Config->associativity = std::stoi(value);
                } else if (key == "prefetch_enabled") {
                    config.hierarchyConfig.l2Config->prefetchEnabled = (value == "1" || value == "true" || value == "yes");
                } else if (key == "prefetch_distance") {
                    config.hierarchyConfig.l2Config->prefetchDistance = std::stoi(value);
                }
            } else if (currentSection == "Prefetching") {
                if (key == "use_stride_prediction") {
                    config.hierarchyConfig.useStridePrediction = (value == "1" || value == "true" || value == "yes");
                } else if (key == "use_adaptive_prefetching") {
                    config.hierarchyConfig.useAdaptivePrefetching = (value == "1" || value == "true" || value == "yes");
                } else if (key == "stride_table_size") {
                    config.hierarchyConfig.strideTableSize = std::stoi(value);
                }
            } else if (currentSection == "General") {
                if (key == "name") {
                    config.name = value;
                } else {
                    // Store any other general parameters in extraOptions
                    config.extraOptions[key] = value;
                }
            } else {
                // Store any other section parameters in extraOptions
                config.extraOptions[currentSection + "." + key] = value;
            }
        }
    }
    
    // Ensure L2 has same block size as L1 if present
    if (config.hierarchyConfig.l2Config) {
        config.hierarchyConfig.l2Config->blockSize = config.hierarchyConfig.l1Config.blockSize;
    }
    
    return config;
}

bool ConfigManager::writeIniConfig(const std::filesystem::path& configPath, const SimulatorConfig& config) {
    std::ofstream file(configPath);
    if (!file) {
        return false;
    }
    
    // Write general section
    file << "[General]\n";
    file << "name = " << config.name << "\n\n";
    
    // Write L1 cache section
    file << "[L1Cache]\n";
    file << "size = " << config.hierarchyConfig.l1Config.size << "\n";
    file << "associativity = " << config.hierarchyConfig.l1Config.associativity << "\n";
    file << "block_size = " << config.hierarchyConfig.l1Config.blockSize << "\n";
    file << "prefetch_enabled = " << (config.hierarchyConfig.l1Config.prefetchEnabled ? "true" : "false") << "\n";
    file << "prefetch_distance = " << config.hierarchyConfig.l1Config.prefetchDistance << "\n\n";
    
    // Write L2 cache section if present
    if (config.hierarchyConfig.l2Config) {
        file << "[L2Cache]\n";
        file << "size = " << config.hierarchyConfig.l2Config->size << "\n";
        file << "associativity = " << config.hierarchyConfig.l2Config->associativity << "\n";
        file << "prefetch_enabled = " << (config.hierarchyConfig.l2Config->prefetchEnabled ? "true" : "false") << "\n";
        file << "prefetch_distance = " << config.hierarchyConfig.l2Config->prefetchDistance << "\n\n";
    } else {
        file << "[L2Cache]\n";
        file << "size = 0\n";
        file << "associativity = 0\n";
        file << "prefetch_enabled = false\n";
        file << "prefetch_distance = 0\n\n";
    }
    
    // Write prefetching section
    file << "[Prefetching]\n";
    file << "use_stride_prediction = " << (config.hierarchyConfig.useStridePrediction ? "true" : "false") << "\n";
    file << "use_adaptive_prefetching = " << (config.hierarchyConfig.useAdaptivePrefetching ? "true" : "false") << "\n";
    file << "stride_table_size = " << config.hierarchyConfig.strideTableSize << "\n\n";
    
    // Write extra options
    std::string lastSection;
    for (const auto& [key, value] : config.extraOptions) {
        // Check if key contains a section delimiter
        auto pos = key.find('.');
        std::string section = (pos != std::string::npos) ? key.substr(0, pos) : "Extra";
        std::string optionKey = (pos != std::string::npos) ? key.substr(pos + 1) : key;
        
        // Write section header if needed
        if (section != lastSection) {
            file << "[" << section << "]\n";
            lastSection = section;
        }
        
        // Write the option based on its type
        file << optionKey << " = ";
        try {
            if (value.type() == typeid(std::string)) {
                file << std::any_cast<std::string>(value);
            } else if (value.type() == typeid(int)) {
                file << std::any_cast<int>(value);
            } else if (value.type() == typeid(bool)) {
                file << (std::any_cast<bool>(value) ? "true" : "false");
            } else if (value.type() == typeid(double)) {
                file << std::any_cast<double>(value);
            } else {
                file << "UNKNOWN_TYPE";
            }
        } catch (const std::bad_any_cast& e) {
            // Handle potential casting errors
            std::cerr << "Error writing option " << optionKey << ": " << e.what() << std::endl;
            file << "ERROR";
        }
        file << "\n";
    }
    
    return true;
}

} // namespace cachesim