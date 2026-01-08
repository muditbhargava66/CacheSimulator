/**
 * @file cli_parser_test.cpp
 * @brief Unit tests for CLI parser module
 * @author Mudit Bhargava
 * @date 2026-01-07
 * @version 1.4.0
 */

#include <cassert>
#include <iostream>
#include <vector>

#include "utils/cli_parser.h"

using namespace cachesim;

/**
 * @class CLIParserTest
 * @brief Test suite for CLI parser functionality
 */
class CLIParserTest {
public:
  static void runAllTests() {
    std::cout << "=== CLI Parser Tests ===" << std::endl;

    testBasicParsing();
    testHelpFlag();
    testVersionFlag();
    testConfigFile();
    testParallelOptions();

    std::cout << "\nAll CLI parser tests passed!" << std::endl;
  }

private:
  static void testBasicParsing() {
    std::cout << "\n[TEST] Basic Parsing..." << std::endl;

    // Create test arguments
    const char *argv[] = {"cachesim", "trace.txt"};
    int argc = 2;

    auto result = CLIParser::parse(argc, const_cast<char **>(argv));
    assert(result.has_value() && "Basic parsing should succeed");

    auto &opts = result.value();
    assert(opts.traceFile == "trace.txt" && "Trace file parsed correctly");
    assert(!opts.help && "Help not set");
    assert(!opts.version && "Version not set");

    std::cout << "  Basic parsing: PASSED" << std::endl;
  }

  static void testHelpFlag() {
    std::cout << "\n[TEST] Help Flag..." << std::endl;

    const char *argv[] = {"cachesim", "--help"};
    int argc = 2;

    auto result = CLIParser::parse(argc, const_cast<char **>(argv));
    assert(result.has_value() && "Help flag parsing succeeded");
    assert(result->help && "Help flag set");

    // Test short form
    const char *argv2[] = {"cachesim", "-h"};
    result = CLIParser::parse(2, const_cast<char **>(argv2));
    assert(result.has_value() && "Short help flag parsing succeeded");
    assert(result->help && "Short help flag set");

    std::cout << "  Help flag: PASSED" << std::endl;
  }

  static void testVersionFlag() {
    std::cout << "\n[TEST] Version Flag..." << std::endl;

    const char *argv[] = {"cachesim", "--version"};
    int argc = 2;

    auto result = CLIParser::parse(argc, const_cast<char **>(argv));
    assert(result.has_value() && "Version flag parsing succeeded");
    assert(result->version && "Version flag set");

    // Test short form
    const char *argv2[] = {"cachesim", "-v"};
    result = CLIParser::parse(2, const_cast<char **>(argv2));
    assert(result.has_value() && "Short version flag parsing succeeded");
    assert(result->version && "Short version flag set");

    std::cout << "  Version flag: PASSED" << std::endl;
  }

  static void testConfigFile() {
    std::cout << "\n[TEST] Config File Option..." << std::endl;

    const char *argv[] = {"cachesim", "-c", "config.json", "trace.txt"};
    int argc = 4;

    auto result = CLIParser::parse(argc, const_cast<char **>(argv));
    assert(result.has_value() && "Config file parsing succeeded");
    assert(result->configFile == "config.json" && "Config file parsed");
    assert(result->traceFile == "trace.txt" && "Trace file parsed");

    // Test long form
    const char *argv2[] = {"cachesim", "--config", "config.json"};
    result = CLIParser::parse(3, const_cast<char **>(argv2));
    assert(result.has_value() && "Long config flag parsing succeeded");
    assert(result->configFile == "config.json" && "Config file parsed");

    std::cout << "  Config file option: PASSED" << std::endl;
  }

  static void testParallelOptions() {
    std::cout << "\n[TEST] Parallel Options..." << std::endl;

    const char *argv[] = {"cachesim", "-p", "4", "trace.txt"};
    int argc = 4;

    auto result = CLIParser::parse(argc, const_cast<char **>(argv));
    assert(result.has_value() && "Parallel parsing succeeded");
    assert(result->parallel && "Parallel flag set");
    assert(result->numThreads == 4 && "Thread count parsed");

    std::cout << "  Parallel options: PASSED" << std::endl;
  }
};

int main() {
  try {
    CLIParserTest::runAllTests();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
