cmake_minimum_required(VERSION 3.20)
project(cryptoquantengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(backtest
  cryptoquantengine/backtest_main.cpp
  cryptoquantengine/core/orderbook/orderbook.cpp
  cryptoquantengine/utils/config/config_reader.cpp
  cryptoquantengine/core/execution_engine/execution_engine.cpp
  cryptoquantengine/core/backtest_engine/backtest_engine.cpp
  cryptoquantengine/core/market_data/market_data_feed.cpp
  cryptoquantengine/core/market_data/readers/base_stream_reader.cpp
  cryptoquantengine/core/market_data/readers/book_stream_reader.cpp
  cryptoquantengine/core/market_data/readers/trade_stream_reader.cpp
  cryptoquantengine/core/recorder/recorder.cpp
  cryptoquantengine/core/strategy/grid_trading/grid_trading.cpp
  cryptoquantengine/utils/logger/logger.cpp
)

target_include_directories(backtest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/cryptoquantengine
  ${CMAKE_CURRENT_SOURCE_DIR}/external 
  ${CMAKE_CURRENT_SOURCE_DIR}/external/websocketpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/json
)

target_compile_options(backtest PRIVATE
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:Debug>:-O3 -Wall -Wextra -Wpedantic>
)

target_link_libraries(backtest PRIVATE Threads::Threads)

add_executable(stream
  cryptoquantengine/wss_main.cpp
  cryptoquantengine/core/market_data/readers/ws/binance_stream_reader.cc
  cryptoquantengine/core/market_data/readers/ws/websocket_stream_reader.cc
)

target_include_directories(stream PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/cryptoquantengine
  ${CMAKE_CURRENT_SOURCE_DIR}/external
  ${CMAKE_CURRENT_SOURCE_DIR}/external/websocketpp
  ${CMAKE_CURRENT_SOURCE_DIR}/external/json
)

target_compile_options(stream PRIVATE
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:Debug>:-O3 -Wall -Wextra -Wpedantic>
)

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
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

target_link_libraries(stream PRIVATE Threads::Threads)
target_link_libraries(stream PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(stream PRIVATE CURL::libcurl)

function(add_hft_test_executable target_name source_files)
  add_executable(${target_name} ${source_files})
  target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cryptoquantengine)
  target_link_libraries(${target_name} PRIVATE Catch2::Catch2WithMain Threads::Threads)
  catch_discover_tests(${target_name})
endfunction()

add_hft_test_executable(test_orderbook 
  "tests/core/test_orderbook.cpp;cryptoquantengine/core/orderbook/orderbook.cpp;cryptoquantengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_config_reader 
  "tests/utils/test_config_reader.cpp;cryptoquantengine/utils/config/config_reader.cpp"
)
add_hft_test_executable(test_bookstream_reader
  "tests/market_data/test_book_stream_reader.cpp;cryptoquantengine/core/market_data/readers/book_stream_reader.cpp;cryptoquantengine/core/market_data/readers/base_stream_reader.cpp;cryptoquantengine/core/orderbook/orderbook.cpp;cryptoquantengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_trade_stream_reader
  "tests/market_data/test_trade_stream_reader.cpp;cryptoquantengine/core/market_data/readers/trade_stream_reader.cpp;cryptoquantengine/core/market_data/readers/base_stream_reader.cpp;cryptoquantengine/core/orderbook/orderbook.cpp;cryptoquantengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_market_data_feed
  "tests/market_data/test_market_data_feed.cpp;cryptoquantengine/core/market_data/market_data_feed.cpp;cryptoquantengine/core/market_data/readers/base_stream_reader.cpp;cryptoquantengine/core/market_data/readers/trade_stream_reader.cpp;cryptoquantengine/core/market_data/readers/book_stream_reader.cpp"	
)
add_hft_test_executable(test_execution_engine "tests/core/test_execution_engine.cpp;cryptoquantengine/core/execution_engine/execution_engine.cpp;cryptoquantengine/core/orderbook/orderbook.cpp;cryptoquantengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_backtest_engine 
  "tests/core/test_backtest_engine.cpp;cryptoquantengine/core/backtest_engine/backtest_engine.cpp;cryptoquantengine/core/execution_engine/execution_engine.cpp;cryptoquantengine/core/orderbook/orderbook.cpp;cryptoquantengine/core/market_data/market_data_feed.cpp;cryptoquantengine/core/market_data/readers/base_stream_reader.cpp;cryptoquantengine/core/market_data/readers/trade_stream_reader.cpp;cryptoquantengine/core/market_data/readers/book_stream_reader.cpp;cryptoquantengine/utils/logger/logger.cpp"
)
add_hft_test_executable(test_stat_utils 
  "tests/utils/test_stat_utils.cpp"
)
add_hft_test_executable(test_recorder 
  "tests/core/test_recorder.cpp;cryptoquantengine/core/recorder/recorder.cpp;cryptoquantengine/core/backtest_engine/backtest_engine.cpp;cryptoquantengine/core/orderbook/orderbook.cpp;cryptoquantengine/core/execution_engine/execution_engine.cpp;cryptoquantengine/core/market_data/market_data_feed.cpp;cryptoquantengine/core/market_data/readers/base_stream_reader.cpp;cryptoquantengine/core/market_data/readers/trade_stream_reader.cpp;cryptoquantengine/core/market_data/readers/book_stream_reader.cpp;cryptoquantengine/utils/logger/logger.cpp"
)
add_hft_test_executable (test_grid_trading 
  "tests/strategies/test_grid_trading.cpp;cryptoquantengine/core/strategy/grid_trading/grid_trading.cpp;cryptoquantengine/core/execution_engine/execution_engine.cpp;cryptoquantengine/core/backtest_engine/backtest_engine.cpp;cryptoquantengine/core/orderbook/orderbook.cpp;cryptoquantengine/core/market_data/market_data_feed.cpp;cryptoquantengine/core/market_data/readers/base_stream_reader.cpp;cryptoquantengine/core/market_data/readers/trade_stream_reader.cpp;cryptoquantengine/core/market_data/readers/book_stream_reader.cpp;cryptoquantengine/utils/logger/logger.cpp"
)
add_hft_test_executable (test_math_utils 
  "tests/utils/test_math_utils.cpp"
)
add_hft_test_executable (test_logger 
  "tests/utils/test_logger.cpp;cryptoquantengine/utils/logger/logger.cpp"
)
