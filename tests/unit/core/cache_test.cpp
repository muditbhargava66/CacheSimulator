#include "../../src/utils/trace_parser.h"
#include <iostream>
#include <sstream>

namespace cachesim {

// We need to use the implementation from the header file
// instead of providing a new implementation here.
// This file should contain tests, not a reimplementation.

// Define a simple test function for TraceParser
void testTraceParser() {
    // Create a simple trace
    std::string tempFilename = "test_trace.txt";
    {
        std::ofstream file(tempFilename);
        file << "r 0x1000\n";
        file << "w 0x2000\n";
        file << "r 0x3000\n";
    }
    
    // Create a parser
    TraceParser parser(tempFilename);
    
    // Check if file is valid
    if (!parser.isValid()) {
        std::cerr << "Failed to open trace file" << std::endl;
        return;
    }
    
    // Get all accesses
    std::vector<MemoryAccess> accesses = parser.parseAll();
    
    // Verify size
    if (accesses.size() != 3) {
        std::cerr << "Wrong number of accesses parsed: " << accesses.size() << std::endl;
        return;
    }
    
    // Verify individual accesses
    if (accesses[0].isWrite || accesses[0].address != 0x1000) {
        std::cerr << "First access parsed incorrectly" << std::endl;
        return;
    }
    
    if (!accesses[1].isWrite || accesses[1].address != 0x2000) {
        std::cerr << "Second access parsed incorrectly" << std::endl;
        return;
    }
    
    if (accesses[2].isWrite || accesses[2].address != 0x3000) {
        std::cerr << "Third access parsed incorrectly" << std::endl;
        return;
    }
    
    // Test getNextAccess separately
    parser.reset();
    
    auto access1 = parser.getNextAccess();
    if (!access1 || access1->isWrite || access1->address != 0x1000) {
        std::cerr << "getNextAccess returned incorrect first access" << std::endl;
        return;
    }
    
    auto access2 = parser.getNextAccess();
    if (!access2 || !access2->isWrite || access2->address != 0x2000) {
        std::cerr << "getNextAccess returned incorrect second access" << std::endl;
        return;
    }
    
    auto access3 = parser.getNextAccess();
    if (!access3 || access3->isWrite || access3->address != 0x3000) {
        std::cerr << "getNextAccess returned incorrect third access" << std::endl;
        return;
    }
    
    auto access4 = parser.getNextAccess();
    if (access4) {
        std::cerr << "getNextAccess should return nullopt at end of file" << std::endl;
        return;
    }
    
    // Test statistics
    if (parser.getTotalAccesses() != 3) {
        std::cerr << "Wrong total accesses count" << std::endl;
        return;
    }
    
    if (parser.getReadAccesses() != 2) {
        std::cerr << "Wrong read accesses count" << std::endl;
        return;
    }
    
    if (parser.getWriteAccesses() != 1) {
        std::cerr << "Wrong write accesses count" << std::endl;
        return;
    }
    
    // Clean up
    std::filesystem::remove(tempFilename);
    
    std::cout << "TraceParser tests passed!" << std::endl;
}

} // namespace cachesim

// Main function to run the tests
int main() {
    try {
        cachesim::testTraceParser();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}