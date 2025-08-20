cmake_minimum_required(VERSION 3.20)
project(hftengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hftbacktest
  hftengine/backtest_main.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/utils/config/config_reader.cpp
  hftengine/core/execution_engine/execution_engine.cpp
  hftengine/core/backtest_engine/backtest_engine.cpp
  hftengine/core/data/market_data_feed.cpp
  hftengine/core/data/readers/book_stream_reader.cpp
  hftengine/core/data/readers/trade_stream_reader.cpp
  hftengine/core/recorder/recorder.cpp
  hftengine/core/strategy/grid_trading.cpp
  hftengine/utils/logger/logger.cpp
)

target_include_directories(hftbacktest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
  ${CMAKE_CURRENT_SOURCE_DIR}/external 
)

target_compile_options(hftbacktest PRIVATE
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:Debug>:-O3 -Wall -Wextra -Wpedantic>
)

# Link pthreads to main executable (required for std::thread on Linux)
target_link_libraries(hftbacktest PRIVATE Threads::Threads)

# Catch2 test setup
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)
enable_testing()

add_executable(test_orderbook
  tests/test_orderbook.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/utils/logger/logger.cpp
)

target_include_directories(test_orderbook PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_config_reader
  tests/utils/test_config_reader.cpp
  hftengine/utils/config/config_reader.cpp
)

target_include_directories(test_config_reader PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_book_stream_reader
  tests/test_book_stream_reader.cpp
  hftengine/core/data/readers/book_stream_reader.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/utils/logger/logger.cpp
)

target_include_directories(test_book_stream_reader PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_trade_stream_reader
  tests/test_trade_stream_reader.cpp
  hftengine/core/data/readers/trade_stream_reader.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/utils/logger/logger.cpp
)

target_include_directories(test_trade_stream_reader PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_market_data_feed
  tests/test_market_data_feed.cpp
  hftengine/core/data/market_data_feed.cpp
  hftengine/core/data/readers/trade_stream_reader.cpp
  hftengine/core/data/readers/book_stream_reader.cpp
)

target_include_directories(test_market_data_feed PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_execution_engine
  tests/test_execution_engine.cpp
  hftengine/core/execution_engine/execution_engine.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/utils/logger/logger.cpp
)

target_include_directories(test_execution_engine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_backtest_engine
  tests/test_backtest_engine.cpp
  hftengine/core/backtest_engine/backtest_engine.cpp
  hftengine/core/execution_engine/execution_engine.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/core/data/market_data_feed.cpp
  hftengine/core/data/readers/trade_stream_reader.cpp
  hftengine/core/data/readers/book_stream_reader.cpp
  hftengine/utils/logger/logger.cpp
)

target_include_directories(test_backtest_engine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_stat_utils
  tests/utils/test_stat_utils.cpp
)

target_include_directories(test_stat_utils PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_recorder
  tests/test_recorder.cpp
  hftengine/core/recorder/recorder.cpp
  hftengine/core/backtest_engine/backtest_engine.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/core/execution_engine/execution_engine.cpp
  hftengine/core/data/market_data_feed.cpp
  hftengine/core/data/readers/trade_stream_reader.cpp
  hftengine/core/data/readers/book_stream_reader.cpp
  hftengine/utils/logger/logger.cpp
)

target_include_directories(test_recorder PRIVATE
  ${CMAKE_CURRENT_SOURDE_DIR}/hftengine
)

add_executable(test_grid_trading
  tests/strategies/test_grid_trading.cpp
  hftengine/core/strategy/grid_trading.cpp
  hftengine/core/execution_engine/execution_engine.cpp
  hftengine/core/backtest_engine/backtest_engine.cpp
  hftengine/core/orderbook/orderbook.cpp
  hftengine/core/data/market_data_feed.cpp
  hftengine/core/data/readers/trade_stream_reader.cpp
  hftengine/core/data/readers/book_stream_reader.cpp
  hftengine/utils/logger/logger.cpp
)
target_include_directories(test_grid_trading PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine)

add_executable(test_math_utils
  tests/utils/test_math_utils.cpp
)
target_include_directories(test_math_utils PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/hftengine
)

add_executable(test_logger
  tests/utils/test_logger.cpp
  hftengine/utils/logger/logger.cpp
)
target_include_directories(test_logger PRIVATE
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
target_link_libraries(test_backtest_engine PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_stat_utils PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_recorder PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_grid_trading PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_math_utils PRIVATE Catch2::Catch2WithMain Threads::Threads)
target_link_libraries(test_logger PRIVATE Catch2::Catch2WithMain Threads::Threads)

# Discover tests for both executables
include(CTest)
include(Catch)
catch_discover_tests(test_orderbook)
catch_discover_tests(test_config_reader)
catch_discover_tests(test_book_stream_reader)
catch_discover_tests(test_trade_stream_reader)
catch_discover_tests(test_market_data_feed)
catch_discover_tests(test_execution_engine)
catch_discover_tests(test_backtest_engine)
catch_discover_tests(test_stat_utils)
catch_discover_tests(test_recorder)
catch_discover_tests(test_grid_trading)
catch_discover_tests(test_math_utils)
catch_discover_tests(test_logger)

add_custom_target(run_all_tests DEPENDS
  test_orderbook
  test_config_reader
  test_book_stream_reader
  test_trade_stream_reader
  test_market_data_feed
  test_execution_engine
  test_backtest_engine
  test_stat_utils
  test_recorder
  test_grid_trading
  test_math_utils
  test_logger
)