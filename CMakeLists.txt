cmake_minimum_required(VERSION 3.14)
project(CacheSimulator VERSION 1.2.0 LANGUAGES CXX)

# Set C++17 as the required standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Generate compile_commands.json for code completion engines
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
    
    # Add debug flags
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0")
    
    # Add release flags
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
elseif(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    
    # Enable exceptions
    add_compile_options(/EHsc)
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/utils
)

# Define source files
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")
file(GLOB MAIN_SOURCES "src/main.cpp")

# Explicitly add new v1.1.0 and v1.2.0 source files to ensure they're included
list(APPEND CORE_SOURCES "src/core/write_policy.cpp")
list(APPEND CORE_SOURCES "src/core/multiprocessor/processor_core.cpp")
list(APPEND CORE_SOURCES "src/core/multiprocessor/coherence_controller.cpp")
list(APPEND CORE_SOURCES "src/core/multiprocessor/interconnect.cpp")
# Note: victim_cache.h, parallel_executor.h are header-only

# Define header files
file(GLOB_RECURSE CORE_HEADERS "src/core/*.h")
file(GLOB_RECURSE UTILS_HEADERS "src/utils/*.h")

# Create a list of all sources
set(ALL_SOURCES ${CORE_SOURCES} ${UTILS_SOURCES} ${MAIN_SOURCES})
set(ALL_HEADERS ${CORE_HEADERS} ${UTILS_HEADERS})

# Define library for tests to link against
add_library(cachesim_lib STATIC ${CORE_SOURCES} ${UTILS_SOURCES})

# Link against threads library for parallel processing
find_package(Threads REQUIRED)
target_link_libraries(cachesim_lib ${CMAKE_THREAD_LIBS_INIT})

# Define main executable
add_executable(cachesim ${MAIN_SOURCES})
target_link_libraries(cachesim cachesim_lib)

# Create trace generator tool
file(GLOB TOOLS_SOURCES "tools/*.cpp")
foreach(TOOL_SOURCE ${TOOLS_SOURCES})
    get_filename_component(TOOL_NAME ${TOOL_SOURCE} NAME_WE)
    add_executable(${TOOL_NAME} ${TOOL_SOURCE})
    target_link_libraries(${TOOL_NAME} cachesim_lib)
    set_target_properties(${TOOL_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tools)
endforeach()

# Enable testing
enable_testing()

# Add unit tests - Core components
file(GLOB CORE_TEST_SOURCES "tests/unit/core/*.cpp")
foreach(TEST_SOURCE ${CORE_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} cachesim_lib)
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/unit/core)
    add_test(NAME unit_core_${TEST_NAME} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/unit/core/${TEST_NAME})
endforeach()

# Add unit tests - Policies
file(GLOB POLICY_TEST_SOURCES "tests/unit/policies/*.cpp")
foreach(TEST_SOURCE ${POLICY_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} cachesim_lib)
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/unit/policies)
    add_test(NAME unit_policies_${TEST_NAME} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/unit/policies/${TEST_NAME})
endforeach()

# Add unit tests - Utils
file(GLOB UTILS_TEST_SOURCES "tests/unit/utils/*.cpp")
foreach(TEST_SOURCE ${UTILS_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} cachesim_lib)
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/unit/utils)
    add_test(NAME unit_utils_${TEST_NAME} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/unit/utils/${TEST_NAME})
endforeach()

# Add integration tests
file(GLOB INTEGRATION_TEST_SOURCES "tests/integration/*.cpp")
foreach(TEST_SOURCE ${INTEGRATION_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} cachesim_lib)
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/integration)
    add_test(NAME integration_${TEST_NAME} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/integration/${TEST_NAME})
endforeach()

# Add performance tests
file(GLOB PERFORMANCE_TEST_SOURCES "tests/performance/*.cpp")
foreach(TEST_SOURCE ${PERFORMANCE_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} cachesim_lib)
    set_target_properties(${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/performance)
    add_test(NAME performance_${TEST_NAME} COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/performance/${TEST_NAME})
endforeach()

# Install targets
install(TARGETS cachesim
    RUNTIME DESTINATION bin
)

# Install trace files
install(DIRECTORY traces/
    DESTINATION share/cachesim/traces
)

# Install scripts
install(DIRECTORY scripts/
    DESTINATION share/cachesim/scripts
    USE_SOURCE_PERMISSIONS
)

# Documentation target if Doxygen is available
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
    set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
    set(DOXYGEN_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    configure_file(${PROJECT_SOURCE_DIR}/docs/Doxyfile.in ${DOXYGEN_CONFIG_FILE} @ONLY)
    
    add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG_FILE}
        MAIN_DEPENDENCY ${DOXYGEN_CONFIG_FILE} ${ALL_HEADERS}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    
    add_custom_target(doc DEPENDS ${DOXYGEN_INDEX_FILE})
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "cachesim")
set(CPACK_PACKAGE_VENDOR "Your Organization")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cache and Memory Hierarchy Simulator")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CacheSimulator ${PROJECT_VERSION}")
include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "Cache Simulator Build Configuration:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Compiler:      ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ Standard:      C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Installation path: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")