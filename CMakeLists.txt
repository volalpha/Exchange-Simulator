cmake_minimum_required(VERSION 3.16)

project(ExchangeSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Main exchange simulator
add_executable(exchange
    src/main.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
)

target_include_directories(exchange PRIVATE include)


# OrderBook tests
add_executable(orderbook_tests
    tests/orderbook_tests.cpp
    src/OrderBook.cpp
)

add_executable(matching_engine_tests
    tests/matchingEngine_tests.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
)

target_include_directories(orderbook_tests PRIVATE include)

add_executable(logger_tests
    tests/logger_tests.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
)
target_include_directories(logger_tests PRIVATE include)
target_compile_definitions(logger_tests PRIVATE ENABLE_LOGGING)

enable_testing()

add_test(NAME OrderBookTests COMMAND orderbook_tests)
add_test(NAME MatchingEngineTests COMMAND matching_engine_tests)
add_test(NAME LoggerTests COMMAND logger_tests)
target_include_directories(matching_engine_tests PRIVATE include)

# Benchmarks (compiled without ENABLE_LOGGING)
add_executable(matching_engine_benchmarks
    benchmarks/matching_engine_benchmarks.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
)
target_include_directories(matching_engine_benchmarks PRIVATE include)