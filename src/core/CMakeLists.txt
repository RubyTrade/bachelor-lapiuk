file(GLOB_RECURSE TRADINGBOT_CORE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.cpp"
)

add_library(TradingBotCore STATIC ${TRADINGBOT_CORE_SOURCES})
add_library(TradingBot::Core ALIAS TradingBotCore)

target_include_directories(TradingBotCore
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(TradingBotCore
  PUBLIC
    ${TRADINGBOT_BOOST_LIBS}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

if(Boost_INCLUDE_DIRS)
  target_include_directories(TradingBotCore SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
endif()

target_compile_definitions(TradingBotCore
  PUBLIC
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
)
