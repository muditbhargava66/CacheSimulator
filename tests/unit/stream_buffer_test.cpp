#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cassert>
#include <memory>
#include <optional>

#include "../../src/core/stream_buffer.h"

using namespace cachesim;

// Test fixture for StreamBuffer tests
class StreamBufferTest {
public:
    // Test basic initialization
    static void testInitialization() {
        std::cout << "Running Stream Buffer initialization test..." << std::endl;
        
        // Create a stream buffer with size 8
        StreamBuffer buffer(8);
        
        // Check initial state
        assert(!buffer.isValid() && "Newly created buffer should not be valid");
        assert(buffer.getLastAccessedIndex() == -1 && "Last accessed index should be -1");
        assert(buffer.getBuffer().size() == 8 && "Buffer size should match constructor parameter");
        assert(buffer.getHits() == 0 && "Initial hits should be 0");
        assert(buffer.getAccesses() == 0 && "Initial accesses should be 0");
        
        std::cout << "Stream Buffer initialization test passed." << std::endl;
    }
    
    // Test prefetching behavior
    static void testPrefetching() {
        std::cout << "Running Stream Buffer prefetching test..." << std::endl;
        
        // Create a stream buffer with size 4
        StreamBuffer buffer(4);
        
        // Prefetch starting at address 0x1000
        buffer.prefetch(0x1000);
        
        // Check buffer state after prefetching
        assert(buffer.isValid() && "Buffer should be valid after prefetching");
        assert(buffer.getLastAccessedIndex() == 0 && "Last accessed index should be 0");
        
        // Check that the correct addresses were prefetched
        const auto& bufferContents = buffer.getBuffer();
        assert(bufferContents[0] == 0x1000 && "First entry should be the prefetch address");
        assert(bufferContents[1] == 0x1001 && "Second entry should be prefetch address + 1");
        assert(bufferContents[2] == 0x1002 && "Third entry should be prefetch address + 2");
        assert(bufferContents[3] == 0x1003 && "Fourth entry should be prefetch address + 3");
        
        std::cout << "Stream Buffer prefetching test passed." << std::endl;
    }
    
    // Test buffer access (hits and misses)
    static void testAccess() {
        std::cout << "Running Stream Buffer access test..." << std::endl;
        
        // Create a stream buffer with size 4
        StreamBuffer buffer(4);
        
        // Prefetch starting at address 0x2000
        buffer.prefetch(0x2000);
        
        // Test accessing addresses in the buffer
        bool hit1 = buffer.access(0x2000); // Should hit at index 0
        bool hit2 = buffer.access(0x2002); // Should hit at index 2
        bool miss1 = buffer.access(0x3000); // Should miss
        
        assert(hit1 && "Access to 0x2000 should be a hit");
        assert(hit2 && "Access to 0x2002 should be a hit");
        assert(!miss1 && "Access to 0x3000 should be a miss");
        
        // Check statistics
        assert(buffer.getHits() == 2 && "Should have 2 hits");
        assert(buffer.getAccesses() == 3 && "Should have 3 accesses");
        assert(std::abs(buffer.getHitRate() - 2.0/3.0) < 0.001 && "Hit rate should be 2/3");
        
        // Check last accessed index
        assert(buffer.getLastAccessedIndex() == 2 && "Last accessed index should be 2");
        
        std::cout << "Stream Buffer access test passed." << std::endl;
    }
    
    // Test buffer shifting
    static void testShift() {
        std::cout << "Running Stream Buffer shift test..." << std::endl;
        
        // Create a stream buffer with size 6
        StreamBuffer buffer(6);
        
        // Prefetch starting at address 0x3000
        buffer.prefetch(0x3000);
        
        // Access an address in the middle of the buffer
        bool hit = buffer.access(0x3002); // Access index 2
        assert(hit && "Access to 0x3002 should hit");
        assert(buffer.getLastAccessedIndex() == 2 && "Last accessed index should be 2");
        
        // Shift buffer (should remove entries 0, 1, and 2)
        buffer.shift();
        
        // Check buffer state after shifting
        assert(buffer.getLastAccessedIndex() == -1 && "Last accessed index should be reset to -1");
        
        // Access addresses that were in the buffer
        bool miss1 = buffer.access(0x3000); // Should miss (shifted out)
        bool miss2 = buffer.access(0x3002); // Should miss (shifted out)
        bool hit2 = buffer.access(0x3003); // Should still hit
        
        assert(!miss1 && "Access to 0x3000 should miss after shift");
        assert(!miss2 && "Access to 0x3002 should miss after shift");
        assert(hit2 && "Access to 0x3003 should still hit");
        
        // Check that the buffer size is preserved
        assert(buffer.getBuffer().size() == 6 && "Buffer size should remain unchanged");
        
        std::cout << "Stream Buffer shift test passed." << std::endl;
    }
    
    // Test buffer statistics
    static void testStatistics() {
        std::cout << "Running Stream Buffer statistics test..." << std::endl;
        
        // Create a stream buffer
        StreamBuffer buffer(8);
        
        // Prefetch and access to generate statistics
        buffer.prefetch(0x4000);
        
        // Mix of hits and misses
        buffer.access(0x4000); // hit
        buffer.access(0x4001); // hit
        buffer.access(0x5000); // miss
        buffer.access(0x4003); // hit
        buffer.access(0x5000); // miss
        
        // Check hits and accesses
        assert(buffer.getHits() == 3 && "Should have 3 hits");
        assert(buffer.getAccesses() == 5 && "Should have 5 accesses");
        assert(std::abs(buffer.getHitRate() - 0.6) < 0.001 && "Hit rate should be 0.6");
        
        // Reset statistics
        buffer.resetStats();
        
        // Check reset values
        assert(buffer.getHits() == 0 && "Hits should be reset to 0");
        assert(buffer.getAccesses() == 0 && "Accesses should be reset to 0");
        assert(buffer.getHitRate() == 0.0 && "Hit rate should be reset to 0");
        
        std::cout << "Stream Buffer statistics test passed." << std::endl;
    }
    
    // Run all tests
    static void runAllTests() {
        std::cout << "Running all Stream Buffer tests..." << std::endl;
        
        testInitialization();
        testPrefetching();
        testAccess();
        testShift();
        testStatistics();
        
        std::cout << "All Stream Buffer tests passed!" << std::endl;
    }
};

int main() {
    try {
        StreamBufferTest::runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}