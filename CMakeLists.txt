cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(network-monitor)

# Add the local CMake modules folder to the CMake search path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Dependencies
# If dependencies are managed with conan, we prepend the current build folder
# to the CMake module path, where CMake looks for dependencies.
if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conaninfo.txt)
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}")
endif()

find_package(Boost 1.74 REQUIRED COMPONENTS system unit_test_framework)
find_package(Filesystem REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# Called before any other target is defined.
enable_testing()

# Static library
set(LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/env.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TransportNetwork.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/FileDownloader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/StompClient.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/StompFrame.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/StompServer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/NetworkMonitor.cpp"
)
add_library(network-monitor STATIC ${LIB_SOURCES})
target_compile_features(network-monitor
    PUBLIC
        cxx_std_17
)
target_include_directories(network-monitor
    PUBLIC
        inc
)
target_link_libraries(network-monitor
    PUBLIC
        Boost::Boost
        OpenSSL::OpenSSL
        std::filesystem
        nlohmann_json::nlohmann_json
        spdlog::spdlog
    PRIVATE
        CURL::CURL
)
target_compile_definitions(network-monitor
    PUBLIC
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        TESTS_NETWORK_LAYOUT_JSON="${CMAKE_CURRENT_SOURCE_DIR}/tests/network-layout.json"
        TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/json-test-files"
)

# Tests
set(TESTS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/WebSocketClientTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/StompClientTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/WebSocketClientMock.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/WebSocketServerTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/WebSocketServerMock.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/StompServerTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/NetworkMonitorTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/StompFrameTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/FileDownloaderTests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/TransportNetworkTests.cpp"
)
add_executable(network-monitor-tests ${TESTS_SOURCES})
target_compile_features(network-monitor-tests
    PRIVATE
        cxx_std_17
)
target_link_libraries(network-monitor-tests
    PRIVATE
        network-monitor
        OpenSSL::OpenSSL
        Boost::Boost
        std::filesystem
        spdlog::spdlog
)
add_test(
    NAME network-monitor-tests
    COMMAND $<TARGET_FILE:network-monitor-tests>
)
# This tells CMake to check for a specific output to verify the test outcome.
# When all unit tests pass, Boost.Test prints "No errors detected".
set_tests_properties(network-monitor-tests PROPERTIES
    PASS_REGULAR_EXPRESSION ".*No errors detected"
)

# Executable
set(EXE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
)
add_executable(network-monitor-exe ${EXE_SOURCES})
target_compile_features(network-monitor-exe
    PRIVATE
        cxx_std_17
)
target_compile_definitions(network-monitor-exe
    PRIVATE
        $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=${WINDOWS_VERSION}>
)
target_compile_options(network-monitor-exe
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_link_libraries(network-monitor-exe
    PRIVATE
        network-monitor
)
add_test(
    NAME network-monitor-exe-tests
    COMMAND $<TARGET_FILE:network-monitor-exe>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)
# These options define the pass/fail condition for the test.
set_tests_properties(network-monitor-exe-tests PROPERTIES
    # This affects the duration of the io_context::run function.
    ENVIRONMENT "LTNM_TIMEOUT_MS=5000"

    # We introduce a timeout just to be safe.
    # This should be longer than LTNM_TIMEOUT_MS.
    TIMEOUT 10

    # We verify the content of the program log output. Any error message is a
    # test failure.
    FAIL_REGULAR_EXPRESSION "\\[error\\]"
)

# Test executables
# We build a test STOMP client and then run it in parallel with the network
# monitor executable. We use an intermediate CMake script to run the two
# executables in parallel.
set(TEST_CLIENT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_client.cpp"
)
add_executable(test-client-exe ${TEST_CLIENT_SOURCES})
target_compile_features(test-client-exe
    PRIVATE
        cxx_std_17
)
target_compile_definitions(test-client-exe
    PRIVATE
        $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=${WINDOWS_VERSION}>
)
target_compile_options(test-client-exe
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
target_link_libraries(test-client-exe
    PRIVATE
        network-monitor
        nlohmann_json::nlohmann_json
        OpenSSL::OpenSSL
        spdlog::spdlog
)
add_test(
    NAME integration-test
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    COMMAND "${CMAKE_COMMAND}"
        -DNETWORK_MONITOR_EXE=$<TARGET_FILE:network-monitor-exe>
        -DTEST_CLIENT_EXE=$<TARGET_FILE:test-client-exe>
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration-test.cmake"
)
# These options define the pass/fail condition for the test.
set_tests_properties(integration-test PROPERTIES
    # This affects the duration of the io_context::run function.
    ENVIRONMENT "LTNM_TIMEOUT_MS=5000"

    # We introduce a timeout just to be safe.
    # This should be longer than LTNM_TIMEOUT_MS.
    TIMEOUT 10

    # We verify the content of the program log output. Any error message is a
    # test failure.
    FAIL_REGULAR_EXPRESSION "\\[error\\]"
)