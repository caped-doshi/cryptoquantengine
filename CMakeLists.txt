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
  hftengine/core/market_data/market_data_feed.cpp
  hftengine/core/market_data/readers/book_stream_reader.cpp
  hftengine/core/market_data/readers/trade_stream_reader.cpp
  hftengine/core/recorder/recorder.cpp
  hftengine/core/strategy/grid_trading/grid_trading.cpp
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
include(Catch)
include(CTest)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

function(add_hft_test_executable target_name source_files)
  add_executable(${target_name} ${source_files})
  target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hftengine)
  target_link_libraries(${target_name} PRIVATE Catch2::Catch2WithMain Threads::Threads)
  catch_discover_tests(${target_name})
endfunction()

add_hft_test_executable(test_orderbook 
  "tests/core/test_orderbook.cpp;hftengine/core/orderbook/orderbook.cpp;hftengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_config_reader 
  "tests/utils/test_config_reader.cpp;hftengine/utils/config/config_reader.cpp"
)
add_hft_test_executable(test_bookstream_reader
  "tests/market_data/test_book_stream_reader.cpp;hftengine/core/market_data/readers/book_stream_reader.cpp;hftengine/core/orderbook/orderbook.cpp;hftengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_trade_stream_reader
  "tests/market_data/test_trade_stream_reader.cpp;hftengine/core/market_data/readers/trade_stream_reader.cpp;hftengine/core/orderbook/orderbook.cpp;hftengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_market_data_feed
  "tests/market_data/test_market_data_feed.cpp;hftengine/core/market_data/market_data_feed.cpp;hftengine/core/market_data/readers/trade_stream_reader.cpp;hftengine/core/market_data/readers/book_stream_reader.cpp"	
)
add_hft_test_executable(test_execution_engine "tests/core/test_execution_engine.cpp;hftengine/core/execution_engine/execution_engine.cpp;hftengine/core/orderbook/orderbook.cpp;hftengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_backtest_engine 
  "tests/core/test_backtest_engine.cpp;hftengine/core/backtest_engine/backtest_engine.cpp;hftengine/core/execution_engine/execution_engine.cpp;hftengine/core/orderbook/orderbook.cpp;hftengine/core/market_data/market_data_feed.cpp;hftengine/core/market_data/readers/trade_stream_reader.cpp;hftengine/core/market_data/readers/book_stream_reader.cpp;hftengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_stat_utils 
  "tests/utils/test_stat_utils.cpp"
)
add_hft_test_executable(test_recorder 
  "tests/core/test_recorder.cpp;hftengine/core/recorder/recorder.cpp;hftengine/core/backtest_engine/backtest_engine.cpp;hftengine/core/orderbook/orderbook.cpp;hftengine/core/execution_engine/execution_engine.cpp;hftengine/core/market_data/market_data_feed.cpp;hftengine/core/market_data/readers/trade_stream_reader.cpp;hftengine/core/market_data/readers/book_stream_reader.cpp;hftengine/utils/logger/logger.cpp"
)
add_hft_test_executable (test_grid_trading 
  "tests/strategies/test_grid_trading.cpp;hftengine/core/strategy/grid_trading/grid_trading.cpp;hftengine/core/execution_engine/execution_engine.cpp;hftengine/core/backtest_engine/backtest_engine.cpp;hftengine/core/orderbook/orderbook.cpp;hftengine/core/market_data/market_data_feed.cpp;hftengine/core/market_data/readers/trade_stream_reader.cpp;hftengine/core/market_data/readers/book_stream_reader.cpp;hftengine/utils/logger/logger.cpp"
)
add_hft_test_executable (test_math_utils 
  "tests/utils/test_math_utils.cpp"
)
add_hft_test_executable (test_logger 
  "tests/utils/test_logger.cpp;hftengine/utils/logger/logger.cpp"
)
