/**
 * @file interconnect_test.cpp
 * @brief Unit tests for Ring and Torus interconnects
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include <cassert>
#include <cmath>
#include <iostream>

#include "core/multiprocessor/interconnect.h"

using namespace cachesim;

/**
 * @class InterconnectTest
 * @brief Test suite for interconnect topologies
 */
class InterconnectTest {
public:
  static void runAllTests() {
    std::cout << "=== Interconnect Tests ===" << std::endl;

    testRingInterconnect();
    testTorusInterconnect();
    testInterconnectFactory();

    std::cout << "\nAll interconnect tests passed!" << std::endl;
  }

private:
  static void testRingInterconnect() {
    std::cout << "\n[TEST] Ring Interconnect..." << std::endl;

    const uint32_t numNodes = 8;
    const uint32_t hopLatency = 2;

    auto ring = InterconnectFactory::create(InterconnectType::Ring, numNodes,
                                            hopLatency);
    assert(ring != nullptr && "Ring interconnect created");

    // Test message from node 0 to node 2 (2 hops clockwise)
    InterconnectMessage msg;
    msg.sourceId = 0;
    msg.destId = 2;
    msg.type = InterconnectMessage::Type::DataTransfer;

    uint32_t latency = ring->sendMessage(msg);
    assert(latency == 2 * hopLatency && "Ring latency 0->2 correct");

    // Test message from node 0 to node 6 (2 hops counter-clockwise)
    msg.destId = 6;
    latency = ring->sendMessage(msg);
    assert(latency == 2 * hopLatency && "Ring latency 0->6 correct (wrap)");

    // Test message from node 0 to node 4 (4 hops either way)
    msg.destId = 4;
    latency = ring->sendMessage(msg);
    assert(latency == 4 * hopLatency && "Ring latency 0->4 correct");

    // Test same node (0 hops)
    msg.destId = 0;
    latency = ring->sendMessage(msg);
    assert(latency == 0 && "Ring latency same node = 0");

    // Verify stats
    auto stats = ring->getStats();
    assert(stats.totalMessages == 4 && "Ring message count correct");

    std::cout << "  Ring routing: PASSED" << std::endl;
    std::cout << "  Ring wrap-around: PASSED" << std::endl;
  }

  static void testTorusInterconnect() {
    std::cout << "\n[TEST] Torus Interconnect..." << std::endl;

    const uint32_t numNodes = 16; // 4x4 torus
    const uint32_t hopLatency = 2;

    auto torus = InterconnectFactory::create(InterconnectType::Torus, numNodes,
                                             hopLatency);
    assert(torus != nullptr && "Torus interconnect created");

    // In a 4x4 torus:
    // Node 0 is at (0,0), Node 3 is at (3,0), Node 12 is at (0,3)

    InterconnectMessage msg;
    msg.sourceId = 0;
    msg.type = InterconnectMessage::Type::DataTransfer;

    // Test 0 to 3: 3 hops in X direction, or 1 hop wrap = 1 hop
    msg.destId = 3;
    uint32_t latency = torus->sendMessage(msg);
    // min(3, 4-3) = 1 hop in X
    assert(latency == 1 * hopLatency && "Torus 0->3 uses wrap-around");

    // Test 0 to 5: node 5 is at (1, 1), Manhattan distance = 2
    msg.destId = 5;
    latency = torus->sendMessage(msg);
    assert(latency == 2 * hopLatency && "Torus 0->5 Manhattan distance");

    // Test 0 to 15: node 15 is at (3,3), wrap-around = (1,1) = 2 hops
    msg.destId = 15;
    latency = torus->sendMessage(msg);
    assert(latency == 2 * hopLatency && "Torus 0->15 uses wrap-around both");

    // Test same node
    msg.destId = 0;
    latency = torus->sendMessage(msg);
    assert(latency == 0 && "Torus same node = 0");

    std::cout << "  Torus routing: PASSED" << std::endl;
    std::cout << "  Torus wrap-around: PASSED" << std::endl;
  }

  static void testInterconnectFactory() {
    std::cout << "\n[TEST] Interconnect Factory..." << std::endl;

    // Test all interconnect types
    auto bus = InterconnectFactory::create(InterconnectType::Bus, 4, 10);
    assert(bus != nullptr && "Bus created");

    auto crossbar =
        InterconnectFactory::create(InterconnectType::Crossbar, 4, 5);
    assert(crossbar != nullptr && "Crossbar created");

    auto mesh = InterconnectFactory::create(InterconnectType::Mesh, 16, 2);
    assert(mesh != nullptr && "Mesh created");

    auto ring = InterconnectFactory::create(InterconnectType::Ring, 8, 2);
    assert(ring != nullptr && "Ring created");

    auto torus = InterconnectFactory::create(InterconnectType::Torus, 16, 2);
    assert(torus != nullptr && "Torus created");

    std::cout << "  Factory creates all types: PASSED" << std::endl;
  }
};

int main() {
  try {
    InterconnectTest::runAllTests();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
