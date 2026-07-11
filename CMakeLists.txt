cmake_minimum_required(VERSION 3.15)
project(binance_service)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED CONFIG)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(binance_service
    main.cpp
    src/aggregator.cpp
    src/binance_client.cpp
    src/json_trade_parser.cpp
    src/trade_queue.cpp
    src/window_stats.cpp
)

target_include_directories(binance_service PRIVATE include)

target_link_libraries(binance_service
    PRIVATE
    Boost::boost
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

enable_testing()
find_package(GTest REQUIRED CONFIG)

add_executable(unit_tests
    tests/test_json_trade_parser.cpp
    tests/test_trade_queue.cpp
    tests/test_window_stats.cpp
    src/json_trade_parser.cpp
    src/trade_queue.cpp
    src/window_stats.cpp
)

target_include_directories(unit_tests PRIVATE include)

target_link_libraries(unit_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    nlohmann_json::nlohmann_json
)

add_test(NAME unit_tests COMMAND unit_tests)