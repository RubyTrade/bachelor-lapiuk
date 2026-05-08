file(GLOB_RECURSE TRADINGBOT_STRATEGY_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
)

add_library(TradingBotStrategy STATIC ${TRADINGBOT_STRATEGY_SOURCES})
add_library(TradingBot::Strategy ALIAS TradingBotStrategy)

target_include_directories(TradingBotStrategy
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(TradingBotStrategy
  PUBLIC
    TradingBotCore
)
