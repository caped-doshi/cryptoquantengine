cmake_minimum_required(VERSION 3.20)
project(hftengine LANGUAGES CXX)

# Set C++ standard before defining targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define executable
add_executable(hftengine
  hftengine/main.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/utils/config/config_reader.cpp
  hftengine/core/execution_engine/execution_engine.cpp
  hftengine/core/trading/backtest_engine.cpp
)

# THEN set properties
target_include_directories(hftengine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
  ${CMAKE_CURRENT_SOURCE_DIR}/external 
)

# Compiler flags
target_compile_options(hftengine PRIVATE
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:Debug>:-O3 -Wall -Wextra -Wpedantic>
)

# Catch2 test setup
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)
enable_testing()

# Test Executables - Separate for each component
# 1. OrderBook Tests
add_executable(test_orderbook
  tests/test_orderbook.cpp
  hftengine/core/orderbook/orderbook.cpp
)

target_include_directories(test_orderbook PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

# 2. ConfigReader Tests
add_executable(test_config_reader
  tests/test_config_reader.cpp
  hftengine/utils/config/config_reader.cpp
)

target_include_directories(test_config_reader PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

# 3. BookStreamReader Tests
add_executable(test_book_stream_reader
  tests/test_book_stream_reader.cpp
  hftengine/core/data/readers/book_stream_reader.cpp
  hftengine/core/orderbook/orderbook.cpp
)

target_include_directories(test_book_stream_reader PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

# 4. TradeStreamReader Tests
add_executable(test_trade_stream_reader
  tests/test_trade_stream_reader.cpp
  hftengine/core/data/readers/trade_stream_reader.cpp
  hftengine/core/orderbook/orderbook.cpp
)

target_include_directories(test_trade_stream_reader PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

# 5. MarketDataFeed Tests
add_executable(test_market_data_feed
  tests/test_market_data_feed.cpp
  hftengine/core/data/readers/market_data_feed.cpp
  hftengine/core/data/readers/trade_stream_reader.cpp
  hftengine/core/data/readers/book_stream_reader.cpp
)

target_include_directories(test_market_data_feed PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

# 6. Execution Engine Tests
add_executable(test_execution_engine
  tests/test_execution_engine.cpp
  hftengine/core/execution_engine/execution_engine.cpp
  hftengine/core/orderbook/orderbook.cpp
)

target_include_directories(test_execution_engine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link  test executables to Catch2
target_link_libraries(test_orderbook PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_config_reader PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_book_stream_reader PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_trade_stream_reader PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_market_data_feed PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_execution_engine PRIVATE Catch2::Catch2WithMain Threads::Threads)

# Discover tests for both executables
include(CTest)
include(Catch)
catch_discover_tests(test_orderbook)
catch_discover_tests(test_config_reader)
catch_discover_tests(test_book_stream_reader)
catch_discover_tests(test_trade_stream_reader)
catch_discover_tests(test_market_data_feed)
catch_discover_tests(test_execution_engine)

# Optional: Add a global test target
add_custom_target(run_all_tests DEPENDS
  test_orderbook
  test_config_reader
  test_book_stream_reader
  test_trade_stream_reader
  test_market_data_feed
  test_execution_engine
)