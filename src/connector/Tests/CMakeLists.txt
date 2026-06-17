enable_testing()

add_executable(MarketDataTests MarketDataTest.cpp)

target_link_libraries(MarketDataTests
    gtest_main
    gtest
    MarketDataConnector
)
