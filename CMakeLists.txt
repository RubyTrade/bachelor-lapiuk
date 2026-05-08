cmake_minimum_required(VERSION 3.16)
project(TradingBot VERSION 1.0)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CPP Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(TRADINGBOT_BUILD_TESTS "Build unit tests" OFF)
option(TRADINGBOT_ENABLE_COVERAGE "Enable code coverage report (tests only)" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# --- Third-party (shared by core; strategy uses only via TradingBotCore)
# Boost.System is header-only since 1.69; Homebrew Boost omits a boost_system CMake package.
find_package(Boost REQUIRED COMPONENTS thread)

find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)

set(TRADINGBOT_BOOST_LIBS "")
if(TARGET Boost::thread)
  set(TRADINGBOT_BOOST_LIBS Boost::thread)
else()
  set(TRADINGBOT_BOOST_LIBS ${Boost_LIBRARIES})
endif()

# --- Libraries: core (infra) → strategy (logic) → app
add_subdirectory(src/core)
add_subdirectory(src/strategy)

# Backward-compatible aliases
add_library(TradingBotLib ALIAS TradingBotCore)
add_library(TradingBot::Lib ALIAS TradingBotCore)

# --- Executable
add_executable(TradingBot
  app/main.cpp
)

target_link_libraries(TradingBot
  PRIVATE
    TradingBotStrategy
)

target_compile_definitions(TradingBot PRIVATE
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
) 

# --- Tests
if(TRADINGBOT_BUILD_TESTS)
  include(CTest)
  enable_testing()

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  FetchContent_MakeAvailable(googletest)

  add_subdirectory(tests)

  if(TRADINGBOT_ENABLE_COVERAGE)
    # Coverage flags
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(TradingBotCore PUBLIC -fprofile-instr-generate -fcoverage-mapping)
      target_link_options(TradingBotCore PUBLIC -fprofile-instr-generate -fcoverage-mapping)
      target_compile_options(TradingBotTests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
      target_link_options(TradingBotTests PRIVATE -fprofile-instr-generate -fcoverage-mapping)

      find_program(LLVM_PROFDATA_EXECUTABLE llvm-profdata)
      find_program(LLVM_COV_EXECUTABLE llvm-cov)
      if(LLVM_PROFDATA_EXECUTABLE AND LLVM_COV_EXECUTABLE)
        add_custom_target(coverage
          COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage-%p.profraw $<TARGET_FILE:TradingBotTests>
          COMMAND ${LLVM_PROFDATA_EXECUTABLE} merge -sparse ${CMAKE_BINARY_DIR}/coverage-*.profraw -o ${CMAKE_BINARY_DIR}/coverage.profdata
          COMMAND ${LLVM_COV_EXECUTABLE} report $<TARGET_FILE:TradingBotTests> -instr-profile=${CMAKE_BINARY_DIR}/coverage.profdata
          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
          DEPENDS TradingBotTests
          USES_TERMINAL
        )
      else()
        message(WARNING "Coverage requested but llvm-profdata/llvm-cov not found; skipping coverage target")
      endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(TradingBotCore PUBLIC --coverage -O0 -g)
      target_link_options(TradingBotCore PUBLIC --coverage)
      target_compile_options(TradingBotTests PRIVATE --coverage -O0 -g)
      target_link_options(TradingBotTests PRIVATE --coverage)

      find_program(GCOVR_EXECUTABLE gcovr)
      if(GCOVR_EXECUTABLE)
        add_custom_target(coverage
          COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure

          COMMAND ${GCOVR_EXECUTABLE}
            -r ${CMAKE_SOURCE_DIR}
            --object-directory ${CMAKE_BINARY_DIR}
            --html --html-details -o coverage.html
            --print-summary
            --exclude '.*_deps/.*'
            --exclude '/usr/.*'
            --gcov-ignore-parse-errors=negative_hits.warn_once_per_file

          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
          DEPENDS TradingBotTests
          USES_TERMINAL
        )
      else()
        message(WARNING "Coverage requested but gcovr not found; skipping coverage target")
      endif()
    else()
      message(WARNING "Coverage requested but unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
  endif()
endif()
