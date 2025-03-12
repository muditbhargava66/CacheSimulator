#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <optional>
#include <variant>
#include <cstdint>
#include <filesystem>

namespace cachesim {

// Structure to represent a memory access operation
struct MemoryAccess {
    bool isWrite;     // true for write, false for read
    uint32_t address; // memory address
    
    // Default constructor
    MemoryAccess() = default;
    
    // Constructor with parameters
    MemoryAccess(bool isWrite, uint32_t address) 
        : isWrite(isWrite), address(address) {}
};

// Enum for parse error types
enum class ParseErrorType {
    InvalidAccessType,
    InvalidAddressFormat,
    UnknownError
};

// Parse result type using std::variant (C++17)
using ParseResult = std::variant<MemoryAccess, ParseErrorType>;

// Class to parse memory trace files with C++17 features
class TraceParser {
public:
    // Constructor with std::filesystem::path (C++17)
    explicit TraceParser(const std::filesystem::path& filepath);
    
    // Destructor
    ~TraceParser();
    
    // Parse all memory accesses in the file
    [[nodiscard]] std::vector<MemoryAccess> parseAll();
    
    // Parse the next memory access in the file using std::optional (C++17)
    [[nodiscard]] std::optional<MemoryAccess> getNextAccess();
    
    // Parse with detailed error reporting
    [[nodiscard]] ParseResult getNextAccessWithError();
    
    // Reset the parser to the beginning of the file
    void reset();
    
    // Get file statistics
    [[nodiscard]] size_t getTotalAccesses() const { return totalAccesses; }
    [[nodiscard]] size_t getReadAccesses() const { return readAccesses; }
    [[nodiscard]] size_t getWriteAccesses() const { return writeAccesses; }
    
    // Check if the file exists and is valid
    [[nodiscard]] bool isValid() const { return fileValid; }
    
    // Get the filename
    [[nodiscard]] std::string getFilename() const { return filepath.filename().string(); }
    
    // Get the full file path
    [[nodiscard]] const std::filesystem::path& getFilepath() const { return filepath; }

private:
    std::filesystem::path filepath;
    std::ifstream traceFile;
    bool fileValid;
    
    // Statistics
    size_t totalAccesses;
    size_t readAccesses;
    size_t writeAccesses;
    size_t lineNumber;
    
    // Parse a single line from the trace file
    ParseResult parseLine(std::string_view line);
    
    // Helper method to parse address in various formats
    std::optional<uint32_t> parseAddress(std::string_view addressStr);
};

} // namespace cachesim