/**
 * @file coherence_protocol_test.cpp
 * @brief Unit tests for coherence protocols (MSI, MESI, MOESI)
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include <cassert>
#include <iostream>
#include <memory>

#include "core/coherence_protocol.h"
#include "core/moesi_protocol.h"
#include "core/msi_protocol.h"


using namespace cachesim;

/**
 * @class CoherenceProtocolTest
 * @brief Test suite for coherence protocols
 */
class CoherenceProtocolTest {
public:
  static void runAllTests() {
    std::cout << "=== Coherence Protocol Tests ===" << std::endl;

    testMSIProtocol();
    testMOESIProtocol();
    testProtocolFactory();

    std::cout << "\nAll coherence protocol tests passed!" << std::endl;
  }

private:
  static void testMSIProtocol() {
    std::cout << "\n[TEST] MSI Protocol..." << std::endl;

    MSIProtocol msi;

    // Test local read from Invalid
    int state = static_cast<int>(MSIState::Invalid);
    state = msi.handleLocalRead(state, false);
    assert(state == static_cast<int>(MSIState::Shared) &&
           "Invalid -> Shared on local read");

    // Test local write from Shared
    state = msi.handleLocalWrite(state);
    assert(state == static_cast<int>(MSIState::Modified) &&
           "Shared -> Modified on local write");

    // Test remote read from Modified
    state = msi.handleRemoteRead(state);
    assert(state == static_cast<int>(MSIState::Shared) &&
           "Modified -> Shared on remote read");

    // Test remote write from Shared
    state = msi.handleRemoteWrite(state);
    assert(state == static_cast<int>(MSIState::Invalid) &&
           "Shared -> Invalid on remote write");

    // Test writeback requirement
    assert(msi.requiresWriteback(static_cast<int>(MSIState::Modified)) &&
           "Modified requires writeback");
    assert(!msi.requiresWriteback(static_cast<int>(MSIState::Shared)) &&
           "Shared does not require writeback");

    std::cout << "  MSI protocol transitions: PASSED" << std::endl;
  }

  static void testMOESIProtocol() {
    std::cout << "\n[TEST] MOESI Protocol..." << std::endl;

    MOESIProtocol moesi;

    // Test local read from Invalid with no shared copy -> Exclusive
    int state = static_cast<int>(MOESIState::Invalid);
    state = moesi.handleLocalRead(state, false);
    assert(state == static_cast<int>(MOESIState::Exclusive) &&
           "Invalid -> Exclusive on local read (no shared)");

    // Test local read from Invalid with shared copy -> Shared
    state = static_cast<int>(MOESIState::Invalid);
    state = moesi.handleLocalRead(state, true);
    assert(state == static_cast<int>(MOESIState::Shared) &&
           "Invalid -> Shared on local read (with shared)");

    // Test local write from Exclusive -> Modified
    state = static_cast<int>(MOESIState::Exclusive);
    state = moesi.handleLocalWrite(state);
    assert(state == static_cast<int>(MOESIState::Modified) &&
           "Exclusive -> Modified on local write");

    // Test remote read from Modified -> Owned (key MOESI feature)
    state = moesi.handleRemoteRead(state);
    assert(state == static_cast<int>(MOESIState::Owned) &&
           "Modified -> Owned on remote read");

    // Test that Owned still requires writeback
    assert(moesi.requiresWriteback(static_cast<int>(MOESIState::Owned)) &&
           "Owned requires writeback");
    assert(moesi.requiresWriteback(static_cast<int>(MOESIState::Modified)) &&
           "Modified requires writeback");
    assert(!moesi.requiresWriteback(static_cast<int>(MOESIState::Exclusive)) &&
           "Exclusive does not require writeback");

    // Test remote write invalidates Owned
    state = moesi.handleRemoteWrite(static_cast<int>(MOESIState::Owned));
    assert(state == static_cast<int>(MOESIState::Invalid) &&
           "Owned -> Invalid on remote write");

    std::cout << "  MOESI protocol transitions: PASSED" << std::endl;
    std::cout << "  MOESI Owned state behavior: PASSED" << std::endl;
  }

  static void testProtocolFactory() {
    std::cout << "\n[TEST] Protocol Factory..." << std::endl;

    // Test MSI creation
    auto msi = CoherenceProtocolBase::create(CoherenceProtocolType::MSI);
    assert(msi != nullptr && "MSI protocol created");
    assert(msi->protocolName() == "MSI" && "MSI name correct");

    // Test MESI creation
    auto mesi = CoherenceProtocolBase::create(CoherenceProtocolType::MESI);
    assert(mesi != nullptr && "MESI protocol created");
    assert(mesi->protocolName() == "MESI" && "MESI name correct");

    // Test MOESI creation
    auto moesi = CoherenceProtocolBase::create(CoherenceProtocolType::MOESI);
    assert(moesi != nullptr && "MOESI protocol created");
    assert(moesi->protocolName() == "MOESI" && "MOESI name correct");

    std::cout << "  Protocol factory: PASSED" << std::endl;
  }
};

int main() {
  try {
    CoherenceProtocolTest::runAllTests();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
