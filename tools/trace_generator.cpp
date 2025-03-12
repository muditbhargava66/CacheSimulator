#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <cmath>
#include <optional>
#include <variant>
#include <functional>

// Define namespace for better organization
namespace trace_generator {

// Using namespace aliases
namespace fs = std::filesystem;

// Enumeration for different access patterns
enum class AccessPattern {
    Sequential,
    Strided,
    Random,
    Looping,
    MixedWithLocality
};

// Structure to store generation parameters
struct GenerationParams {
    std::string outputFile = "trace.txt";
    size_t numAccesses = 1000;
    AccessPattern pattern = AccessPattern::Sequential;
    uint32_t startAddress = 0x1000;
    uint32_t endAddress = 0x100000;
    uint32_t stride = 64;           // Stride for strided pattern
    double writeRatio = 0.3;        // Percentage of writes
    size_t loopSize = 10;           // Size of loop for looping pattern
    size_t repetitions = 5;         // Number of repetitions for looping pattern
    size_t localityRegions = 5;     // Number of locality regions for mixed pattern
    uint32_t regionSize = 4096;     // Size of each region in bytes
    double localityProbability = 0.8; // Probability of staying in the current region
    
    // Optional seed for random number generator (for reproducibility)
    std::optional<uint32_t> seed;
    
    // Only include selected patterns in mixed mode
    std::vector<AccessPattern> includedPatterns = {
        AccessPattern::Sequential,
        AccessPattern::Strided,
        AccessPattern::Random,
        AccessPattern::Looping
    };
};

// Generate a single memory address based on the pattern and parameters
uint32_t generateAddress(AccessPattern pattern, size_t index, const GenerationParams& params,
                         std::mt19937& rng, std::vector<uint32_t>& state) {
    switch (pattern) {
        case AccessPattern::Sequential: {
            // Simple sequential pattern
            return params.startAddress + (index * params.stride);
        }
        
        case AccessPattern::Strided: {
            // Strided pattern with configurable stride
            return params.startAddress + (index * params.stride * 2);
        }
        
        case AccessPattern::Random: {
            // Uniform random distribution between start and end
            std::uniform_int_distribution<uint32_t> dist(params.startAddress, 
                                                      params.endAddress - 1);
            // Align to stride boundary
            return (dist(rng) / params.stride) * params.stride;
        }
        
        case AccessPattern::Looping: {
            // Looping pattern repeats a sequence
            size_t position = index % params.loopSize;
            size_t iteration = index / params.loopSize;
            
            // If first iteration or state is empty, generate new address
            if (iteration == 0 || state.size() <= position) {
                uint32_t address = params.startAddress + (position * params.stride);
                
                // Store address in state if first iteration
                if (iteration == 0) {
                    state.push_back(address);
                }
                
                return address;
            } else {
                // Return previously stored address
                return state[position];
            }
        }
        
        case AccessPattern::MixedWithLocality: {
            // Mixed access pattern with locality
            
            // If state is empty, initialize with region information
            if (state.empty()) {
                // Store current region index and reference address
                state = {0, params.startAddress};
            }
            
            uint32_t currentRegion = state[0];
            uint32_t lastAddress = state[1];
            
            // Decide whether to stay in current region or switch
            std::uniform_real_distribution<double> probDist(0.0, 1.0);
            bool stayInRegion = probDist(rng) < params.localityProbability;
            
            if (!stayInRegion) {
                // Switch to a different region
                std::uniform_int_distribution<uint32_t> regionDist(0, params.localityRegions - 1);
                currentRegion = regionDist(rng);
                state[0] = currentRegion;
            }
            
            // Calculate region base address
            uint32_t regionBase = params.startAddress + (currentRegion * params.regionSize);
            
            // Generate address within the region
            uint32_t address;
            
            if (stayInRegion && (index > 0) && (probDist(rng) < 0.7)) {
                // 70% chance of accessing nearby addresses if staying in region
                std::normal_distribution<double> nearbyDist(0, params.stride * 2);
                int32_t offset = static_cast<int32_t>(nearbyDist(rng));
                // Clamp to region boundaries
                offset = std::max<int32_t>(0, std::min<int32_t>(offset, params.regionSize - params.stride));
                // Align to stride
                offset = (offset / params.stride) * params.stride;
                
                address = lastAddress + offset;
                
                // Ensure address is within region
                if (address < regionBase || address >= regionBase + params.regionSize) {
                    address = regionBase + (std::abs(offset) % params.regionSize);
                }
            } else {
                // Uniform distribution within region
                std::uniform_int_distribution<uint32_t> addrDist(0, params.regionSize - params.stride);
                uint32_t offset = (addrDist(rng) / params.stride) * params.stride;
                address = regionBase + offset;
            }
            
            // Update last address
            state[1] = address;
            
            return address;
        }
        
        default:
            // Default to sequential
            return params.startAddress + (index * params.stride);
    }
}

// Generate a memory trace file with the specified parameters
bool generateTrace(const GenerationParams& params) {
    try {
        // Open output file
        std::ofstream outFile(params.outputFile);
        if (!outFile) {
            std::cerr << "Error: Could not open output file " << params.outputFile << std::endl;
            return false;
        }
        
        // Initialize random number generator
        std::random_device rd;
        std::mt19937 rng(params.seed.value_or(rd()));
        
        // Distribution for determining read/write
        std::uniform_real_distribution<double> writeDist(0.0, 1.0);
        
        // Vector to store state for patterns that need it (like looping)
        std::vector<uint32_t> state;
        
        // Vectors for each mixed pattern type
        std::vector<std::vector<uint32_t>> mixedStates;
        if (params.pattern == AccessPattern::MixedWithLocality) {
            mixedStates.resize(params.includedPatterns.size());
        }
        
        // Generate memory accesses
        for (size_t i = 0; i < params.numAccesses; ++i) {
            // Determine read or write
            bool isWrite = writeDist(rng) < params.writeRatio;
            
            // Generate address based on pattern
            uint32_t address;
            
            if (params.pattern == AccessPattern::MixedWithLocality) {
                // For mixed pattern, choose one of the included patterns
                std::uniform_int_distribution<size_t> patternDist(0, params.includedPatterns.size() - 1);
                size_t patternIndex = patternDist(rng);
                auto pattern = params.includedPatterns[patternIndex];
                
                // Generate address using the selected pattern
                address = generateAddress(pattern, i, params, rng, mixedStates[patternIndex]);
            } else {
                // Use the main pattern
                address = generateAddress(params.pattern, i, params, rng, state);
            }
            
            // Write to file
            outFile << (isWrite ? "w " : "r ") << "0x" << std::hex << address << std::dec << std::endl;
        }
        
        outFile.close();
        
        std::cout << "Generated " << params.numAccesses << " memory accesses in " 
                  << params.outputFile << std::endl;
        
        // Print pattern information
        std::cout << "Pattern: ";
        switch (params.pattern) {
            case AccessPattern::Sequential:
                std::cout << "Sequential (stride=" << params.stride << ")";
                break;
            case AccessPattern::Strided:
                std::cout << "Strided (stride=" << (params.stride * 2) << ")";
                break;
            case AccessPattern::Random:
                std::cout << "Random";
                break;
            case AccessPattern::Looping:
                std::cout << "Looping (size=" << params.loopSize 
                          << ", repetitions=" << params.repetitions << ")";
                break;
            case AccessPattern::MixedWithLocality:
                std::cout << "Mixed with Locality (regions=" << params.localityRegions 
                          << ", region size=" << params.regionSize << ")";
                break;
        }
        std::cout << std::endl;
        
        std::cout << "Write ratio: " << (params.writeRatio * 100) << "%" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error generating trace: " << e.what() << std::endl;
        return false;
    }
}

// Helper function to print usage
void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -o, --output <file>        Output trace file (default: trace.txt)" << std::endl;
    std::cout << "  -n, --num <count>          Number of memory accesses (default: 1000)" << std::endl;
    std::cout << "  -p, --pattern <pattern>    Access pattern (default: sequential)" << std::endl;
    std::cout << "                             Patterns: sequential, strided, random, looping, mixed" << std::endl;
    std::cout << "  -s, --start <address>      Start address in hex (default: 0x1000)" << std::endl;
    std::cout << "  -e, --end <address>        End address in hex (default: 0x100000)" << std::endl;
    std::cout << "  --stride <value>           Stride in bytes (default: 64)" << std::endl;
    std::cout << "  -w, --write <ratio>        Write ratio (0.0-1.0) (default: 0.3)" << std::endl;
    std::cout << "  --loop-size <count>        Loop size for looping pattern (default: 10)" << std::endl;
    std::cout << "  --repetitions <count>      Repetitions for looping pattern (default: 5)" << std::endl;
    std::cout << "  --regions <count>          Number of regions for mixed pattern (default: 5)" << std::endl;
    std::cout << "  --region-size <size>       Size of each region in bytes (default: 4096)" << std::endl;
    std::cout << "  --locality <probability>   Locality probability (0.0-1.0) (default: 0.8)" << std::endl;
    std::cout << "  --seed <value>             Random seed for reproducibility" << std::endl;
    std::cout << "  -h, --help                 Display this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " -o sequential.txt -p sequential -n 5000" << std::endl;
    std::cout << "  " << programName << " -o random.txt -p random --stride 128 -w 0.5" << std::endl;
    std::cout << "  " << programName << " -o loop.txt -p looping --loop-size 20 --repetitions 10" << std::endl;
}

// Helper for parsing command line arguments
std::optional<GenerationParams> parseCommandLine(int argc, char* argv[]) {
    if (argc <= 1) {
        printUsage(argv[0]);
        return GenerationParams{};  // Return default parameters
    }
    
    GenerationParams params;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return std::nullopt;
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                params.outputFile = argv[++i];
            }
        } else if (arg == "-n" || arg == "--num") {
            if (i + 1 < argc) {
                params.numAccesses = std::stoul(argv[++i]);
            }
        } else if (arg == "-p" || arg == "--pattern") {
            if (i + 1 < argc) {
                std::string pattern = argv[++i];
                if (pattern == "sequential") {
                    params.pattern = AccessPattern::Sequential;
                } else if (pattern == "strided") {
                    params.pattern = AccessPattern::Strided;
                } else if (pattern == "random") {
                    params.pattern = AccessPattern::Random;
                } else if (pattern == "looping") {
                    params.pattern = AccessPattern::Looping;
                } else if (pattern == "mixed") {
                    params.pattern = AccessPattern::MixedWithLocality;
                } else {
                    std::cerr << "Error: Unknown pattern '" << pattern << "'" << std::endl;
                    return std::nullopt;
                }
            }
        } else if (arg == "-s" || arg == "--start") {
            if (i + 1 < argc) {
                params.startAddress = std::stoul(argv[++i], nullptr, 0); // Support hex with 0x prefix
            }
        } else if (arg == "-e" || arg == "--end") {
            if (i + 1 < argc) {
                params.endAddress = std::stoul(argv[++i], nullptr, 0);
            }
        } else if (arg == "--stride") {
            if (i + 1 < argc) {
                params.stride = std::stoul(argv[++i]);
            }
        } else if (arg == "-w" || arg == "--write") {
            if (i + 1 < argc) {
                params.writeRatio = std::stod(argv[++i]);
                if (params.writeRatio < 0.0 || params.writeRatio > 1.0) {
                    std::cerr << "Error: Write ratio must be between 0.0 and 1.0" << std::endl;
                    return std::nullopt;
                }
            }
        } else if (arg == "--loop-size") {
            if (i + 1 < argc) {
                params.loopSize = std::stoul(argv[++i]);
            }
        } else if (arg == "--repetitions") {
            if (i + 1 < argc) {
                params.repetitions = std::stoul(argv[++i]);
            }
        } else if (arg == "--regions") {
            if (i + 1 < argc) {
                params.localityRegions = std::stoul(argv[++i]);
            }
        } else if (arg == "--region-size") {
            if (i + 1 < argc) {
                params.regionSize = std::stoul(argv[++i]);
            }
        } else if (arg == "--locality") {
            if (i + 1 < argc) {
                params.localityProbability = std::stod(argv[++i]);
                if (params.localityProbability < 0.0 || params.localityProbability > 1.0) {
                    std::cerr << "Error: Locality probability must be between 0.0 and 1.0" << std::endl;
                    return std::nullopt;
                }
            }
        } else if (arg == "--seed") {
            if (i + 1 < argc) {
                params.seed = std::stoul(argv[++i]);
            }
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
            printUsage(argv[0]);
            return std::nullopt;
        }
    }
    
    return params;
}

// Generate all standard trace patterns at once
bool generateStandardTraces(const std::string& directory) {
    try {
        // Create directory if it doesn't exist
        fs::create_directories(directory);
        
        // Base parameters
        GenerationParams params;
        params.numAccesses = 1000;
        
        // Sequential trace
        params.pattern = AccessPattern::Sequential;
        params.outputFile = directory + "/trace_sequential.txt";
        if (!generateTrace(params)) {
            return false;
        }
        
        // Strided trace
        params.pattern = AccessPattern::Strided;
        params.stride = 64;
        params.outputFile = directory + "/trace_strided.txt";
        if (!generateTrace(params)) {
            return false;
        }
        
        // Random trace
        params.pattern = AccessPattern::Random;
        params.outputFile = directory + "/trace_random.txt";
        if (!generateTrace(params)) {
            return false;
        }
        
        // Looping trace
        params.pattern = AccessPattern::Looping;
        params.loopSize = 20;
        params.repetitions = 10;
        params.outputFile = directory + "/trace_looping.txt";
        if (!generateTrace(params)) {
            return false;
        }
        
        // Mixed with locality trace
        params.pattern = AccessPattern::MixedWithLocality;
        params.localityRegions = 5;
        params.regionSize = 4096;
        params.outputFile = directory + "/trace_mixed.txt";
        if (!generateTrace(params)) {
            return false;
        }
        
        // Write-heavy trace (80% writes)
        params.pattern = AccessPattern::Sequential;
        params.writeRatio = 0.8;
        params.outputFile = directory + "/trace_write_heavy.txt";
        if (!generateTrace(params)) {
            return false;
        }
        
        // Read-only trace (0% writes)
        params.pattern = AccessPattern::Sequential;
        params.writeRatio = 0.0;
        params.outputFile = directory + "/trace_read_only.txt";
        if (!generateTrace(params)) {
            return false;
        }
        
        std::cout << "Successfully generated all standard trace patterns in " 
                  << directory << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error generating standard traces: " << e.what() << std::endl;
        return false;
    }
}

} // namespace trace_generator

int main(int argc, char* argv[]) {
    try {
        // Special case for generating all standard traces
        if (argc > 1 && std::string(argv[1]) == "--generate-all") {
            std::string directory = (argc > 2) ? argv[2] : "traces";
            return trace_generator::generateStandardTraces(directory) ? 0 : 1;
        }
        
        // Parse command line arguments
        auto params = trace_generator::parseCommandLine(argc, argv);
        
        // If help was requested or parsing failed, exit
        if (!params) {
            return (argc > 1 && std::string(argv[1]) == "-h") ? 0 : 1;
        }
        
        // Generate trace file
        bool success = trace_generator::generateTrace(*params);
        
        return success ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}