#include "trace_parser.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <iostream>
#include <sstream>
#include <regex>

namespace cachesim {

TraceParser::TraceParser(const std::filesystem::path& filepath)
    : filepath(filepath), 
      fileValid(false), 
      traceFormat(TraceFormat::Simple),
      totalAccesses(0), 
      readAccesses(0), 
      writeAccesses(0), 
      lineNumber(0) {
    
    // Attempt to open the trace file
    traceFile.open(filepath);
    if (traceFile.is_open()) {
        fileValid = true;
        // Auto-detect format on initialization
        traceFormat = detectFormat();
        // Reset to beginning after format detection
        reset();
    } else {
        std::cerr << "Error: Failed to open trace file: " << filepath << std::endl;
    }
}

TraceParser::~TraceParser() {
    if (traceFile.is_open()) {
        traceFile.close();
    }
}

std::vector<MemoryAccess> TraceParser::parseAll() {
    std::vector<MemoryAccess> accesses;
    
    // Reset to the beginning
    reset();
    
    // Parse all accesses
    std::optional<MemoryAccess> access;
    while ((access = getNextAccess())) {
        accesses.push_back(*access);
    }
    
    return accesses;
}

std::optional<MemoryAccess> TraceParser::getNextAccess() {
    if (!fileValid || !traceFile.is_open()) {
        return std::nullopt;
    }
    
    std::string line;
    
    // Read lines until we find a valid access or reach EOF
    while (std::getline(traceFile, line)) {
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse the line based on format
        ParseResult result = (traceFormat == TraceFormat::JSON) ? 
                            parseJSONLine(line) : parseLine(line);
        
        if (auto* access = std::get_if<MemoryAccess>(&result)) {
            totalAccesses++;
            if (access->isWrite) {
                writeAccesses++;
            } else {
                readAccesses++;
            }
            return *access;
        } else {
            // Handle parse error
            auto error = std::get<ParseErrorType>(result);
            std::cerr << "Parse error at line " << lineNumber << ": ";
            switch (error) {
                case ParseErrorType::InvalidAccessType:
                    std::cerr << "Invalid access type" << std::endl;
                    break;
                case ParseErrorType::InvalidAddressFormat:
                    std::cerr << "Invalid address format" << std::endl;
                    break;
                case ParseErrorType::InvalidJSONFormat:
                    std::cerr << "Invalid JSON format" << std::endl;
                    break;
                case ParseErrorType::UnknownError:
                default:
                    std::cerr << "Unknown error" << std::endl;
                    break;
            }
        }
    }
    
    return std::nullopt;
}

ParseResult TraceParser::getNextAccessWithError() {
    if (!fileValid || !traceFile.is_open()) {
        return ParseErrorType::UnknownError;
    }
    
    std::string line;
    
    // Read lines until we find a valid access or reach EOF
    while (std::getline(traceFile, line)) {
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse the line based on format
        ParseResult result = (traceFormat == TraceFormat::JSON) ? 
                            parseJSONLine(line) : parseLine(line);
        
        if (auto* access = std::get_if<MemoryAccess>(&result)) {
            totalAccesses++;
            if (access->isWrite) {
                writeAccesses++;
            } else {
                readAccesses++;
            }
        }
        
        return result;
    }
    
    return ParseErrorType::UnknownError;
}

void TraceParser::reset() {
    if (traceFile.is_open()) {
        traceFile.clear();
        traceFile.seekg(0, std::ios::beg);
        lineNumber = 0;
    }
}

TraceFormat TraceParser::detectFormat() {
    if (!fileValid || !traceFile.is_open()) {
        return TraceFormat::Simple;
    }
    
    // Save current position
    auto currentPos = traceFile.tellg();
    traceFile.seekg(0, std::ios::beg);
    
    std::string line;
    TraceFormat detectedFormat = TraceFormat::Simple;
    
    // Read first few non-comment lines to detect format
    int linesChecked = 0;
    while (std::getline(traceFile, line) && linesChecked < 10) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Check for JSON format (starts with '{')
        if (line[0] == '{') {
            detectedFormat = TraceFormat::JSON;
            break;
        }
        
        // Check for extended format (has more than 2 tokens)
        std::istringstream iss(line);
        std::string token1, token2, token3;
        if (iss >> token1 >> token2 >> token3) {
            detectedFormat = TraceFormat::Extended;
            break;
        }
        
        linesChecked++;
    }
    
    // Restore position
    traceFile.clear();
    traceFile.seekg(currentPos);
    
    return detectedFormat;
}

ParseResult TraceParser::parseLine(std::string_view line) {
    std::string lineStr(line);
    std::istringstream iss(lineStr);
    std::string accessType;
    std::string addressStr;
    
    // Parse the access type and address
    if (!(iss >> accessType >> addressStr)) {
        return ParseErrorType::UnknownError;
    }
    
    // Validate access type
    bool isWrite;
    if (accessType == "r" || accessType == "R") {
        isWrite = false;
    } else if (accessType == "w" || accessType == "W") {
        isWrite = true;
    } else {
        return ParseErrorType::InvalidAccessType;
    }
    
    // Parse the address
    auto address = parseAddress(addressStr);
    if (!address) {
        return ParseErrorType::InvalidAddressFormat;
    }
    
    return MemoryAccess(isWrite, *address);
}

ParseResult TraceParser::parseJSONLine(std::string_view line) {
    // Simple JSON parser for format: {"type": "r|w", "address": "0xHEX"}
    // Using escaped string instead of raw string literal to avoid compilation issues
    std::regex jsonRegex("\\{\\s*\"type\"\\s*:\\s*\"([rRwW])\"\\s*,\\s*\"address\"\\s*:\\s*\"(0x[0-9a-fA-F]+)\"\\s*\\}");
    std::smatch matches;
    std::string lineStr(line);
    
    if (std::regex_match(lineStr, matches, jsonRegex)) {
        // Extract type and address
        std::string typeStr = matches[1].str();
        std::string addressStr = matches[2].str();
        
        bool isWrite = (typeStr == "w" || typeStr == "W");
        
        auto address = parseAddress(addressStr);
        if (!address) {
            return ParseErrorType::InvalidAddressFormat;
        }
        
        return MemoryAccess(isWrite, *address);
    }
    
    return ParseErrorType::InvalidJSONFormat;
}

std::optional<uint32_t> TraceParser::parseAddress(std::string_view addressStr) {
    // Remove any whitespace
    std::string cleanAddr;
    for (char c : addressStr) {
        if (!std::isspace(c)) {
            cleanAddr += c;
        }
    }
    
    // Handle hexadecimal addresses (0x prefix)
    if (cleanAddr.size() > 2 && cleanAddr[0] == '0' && 
        (cleanAddr[1] == 'x' || cleanAddr[1] == 'X')) {
        
        // Parse hex address
        uint32_t address = 0;
        const char* start = cleanAddr.c_str() + 2;
        const char* end = cleanAddr.c_str() + cleanAddr.size();
        
        auto result = std::from_chars(start, end, address, 16);
        if (result.ec == std::errc() && result.ptr == end) {
            return address;
        }
    }
    
    // Try parsing as decimal
    uint32_t address = 0;
    auto result = std::from_chars(cleanAddr.c_str(), 
                                 cleanAddr.c_str() + cleanAddr.size(), 
                                 address, 10);
    if (result.ec == std::errc()) {
        return address;
    }
    
    return std::nullopt;
}

} // namespace cachesim
