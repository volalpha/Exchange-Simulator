cmake_minimum_required(VERSION 3.16)

project(ExchangeSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type set to Release if unspecified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# General compiler options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Main exchange simulator executable
add_executable(exchange
    src/main.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
)
target_include_directories(exchange PRIVATE include)

# Unit test executables
add_executable(orderbook_tests
    tests/orderbook_tests.cpp
    src/OrderBook.cpp
)
target_include_directories(orderbook_tests PRIVATE include)

add_executable(matching_engine_tests
    tests/matchingEngine_tests.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
)
target_include_directories(matching_engine_tests PRIVATE include)

add_executable(logger_tests
    tests/logger_tests.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
)
target_include_directories(logger_tests PRIVATE include)
target_compile_definitions(logger_tests PRIVATE ENABLE_LOGGING)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(orderbook_tests PRIVATE -UNDEBUG)
    target_compile_options(matching_engine_tests PRIVATE -UNDEBUG)
    target_compile_options(logger_tests PRIVATE -UNDEBUG)
endif()

# Register CTest unit tests
enable_testing()
add_test(NAME OrderBookTests COMMAND orderbook_tests)
add_test(NAME MatchingEngineTests COMMAND matching_engine_tests)
add_test(NAME LoggerTests COMMAND logger_tests)

# Benchmark executable (compiled without ENABLE_LOGGING, tuned for native host CPU)
add_executable(matching_engine_benchmarks
    benchmarks/matching_engine_benchmarks.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
)
target_include_directories(matching_engine_benchmarks PRIVATE include)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(matching_engine_benchmarks PRIVATE -O3 -march=native)

    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        set_property(TARGET matching_engine_benchmarks PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()