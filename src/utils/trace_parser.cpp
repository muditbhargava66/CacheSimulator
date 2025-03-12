#include "trace_parser.h"
#include <algorithm>
#include <charconv>
#include <iostream>
#include <sstream>
#include <system_error>

namespace cachesim {

TraceParser::TraceParser(const std::filesystem::path& filepath)
    : filepath(filepath), 
      fileValid(false),
      totalAccesses(0), 
      readAccesses(0), 
      writeAccesses(0),
      lineNumber(0) {
    
    // Check if file exists using C++17 filesystem
    if (!std::filesystem::exists(filepath)) {
        std::cerr << "Error: Trace file " << filepath << " does not exist." << std::endl;
        return;
    }
    
    // Open the file
    traceFile.open(filepath);
    if (!traceFile.is_open()) {
        std::cerr << "Error: Could not open trace file " << filepath << std::endl;
        return;
    }
    
    fileValid = true;
}

TraceParser::~TraceParser() {
    if (traceFile.is_open()) {
        traceFile.close();
    }
}

std::vector<MemoryAccess> TraceParser::parseAll() {
    if (!fileValid) {
        return {};
    }
    
    std::vector<MemoryAccess> accesses;
    
    // Reset to beginning of file
    reset();
    
    // Read all accesses using C++17 optional
    while (auto access = getNextAccess()) {
        accesses.push_back(*access);
    }
    
    return accesses;
}

std::optional<MemoryAccess> TraceParser::getNextAccess() {
    // Use std::variant and std::visit (C++17) to handle the result
    if (auto result = getNextAccessWithError(); std::holds_alternative<MemoryAccess>(result)) {
        return std::get<MemoryAccess>(result);
    }
    return std::nullopt;
}

ParseResult TraceParser::getNextAccessWithError() {
    if (!fileValid || traceFile.eof()) {
        return ParseErrorType::UnknownError;
    }
    
    std::string line;
    while (std::getline(traceFile, line)) {
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        return parseLine(line);
    }
    
    return ParseErrorType::UnknownError;
}

void TraceParser::reset() {
    if (traceFile.is_open()) {
        traceFile.clear();
        traceFile.seekg(0, std::ios::beg);
    } else if (fileValid) {
        traceFile.open(filepath);
    }
    
    // Reset statistics
    totalAccesses = 0;
    readAccesses = 0;
    writeAccesses = 0;
    lineNumber = 0;
}

ParseResult TraceParser::parseLine(std::string_view line) {
    // Using C++17 string_view to avoid copies
    line = line.substr(line.find_first_not_of(" \t"));
    
    if (line.empty()) {
        return ParseErrorType::InvalidAccessType;
    }
    
    // Extract access type
    char accessType = line[0];
    if (accessType != 'r' && accessType != 'w') {
        return ParseErrorType::InvalidAccessType;
    }
    
    // Find address part (skip whitespace)
    auto addrStart = line.find_first_not_of(" \t", 1);
    if (addrStart == std::string_view::npos) {
        return ParseErrorType::InvalidAddressFormat;
    }
    
    // Extract address
    auto addrStr = line.substr(addrStart);
    
    // Parse address with error handling
    auto address = parseAddress(addrStr);
    if (!address) {
        return ParseErrorType::InvalidAddressFormat;
    }
    
    // Create and return memory access
    bool isWrite = (accessType == 'w');
    
    // Update statistics
    totalAccesses++;
    if (isWrite) {
        writeAccesses++;
    } else {
        readAccesses++;
    }
    
    return MemoryAccess{isWrite, *address};
}

std::optional<uint32_t> TraceParser::parseAddress(std::string_view addressStr) {
    // Using C++17 techniques for efficient string parsing
    
    // Remove whitespace
    addressStr = addressStr.substr(0, addressStr.find_first_of(" \t"));
    
    if (addressStr.empty()) {
        return std::nullopt;
    }
    
    try {
        // Check for hex prefix
        if (addressStr.size() >= 2 && addressStr.substr(0, 2) == "0x") {
            // C++17 from_chars for hex parsing
            uint32_t result = 0;
            auto sv = addressStr.substr(2); // Skip "0x"
            std::string str(sv);  // Unfortunately from_chars needs a char*
            
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result, 16);
            if (ec == std::errc()) {
                return result;
            }
        } else {
            // Try decimal first
            uint32_t result = 0;
            std::string str(addressStr);
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result, 10);
            
            if (ec == std::errc()) {
                return result;
            }
            
            // Try hex without prefix
            result = 0;
            auto [ptr2, ec2] = std::from_chars(str.data(), str.data() + str.size(), result, 16);
            if (ec2 == std::errc()) {
                return result;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing address '" << addressStr 
                  << "': " << e.what() << std::endl;
    }
    
    return std::nullopt;
}

} // namespace cachesim