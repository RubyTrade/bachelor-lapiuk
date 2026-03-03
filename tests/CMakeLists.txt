# Automatically find all test files
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(TradingBotTests ${TEST_SOURCES})

target_link_libraries(TradingBotTests
  PRIVATE
    TradingBotLib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(TradingBotTests)
