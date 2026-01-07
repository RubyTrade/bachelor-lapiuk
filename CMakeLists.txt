cmake_minimum_required(VERSION 3.16)
project(TradingBot VERSION 1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CPP Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Sources
file(GLOB_RECURSE SRC_FILES src/*.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(TradingBot ${SRC_FILES})

target_include_directories(TradingBot PRIVATE 
  ${CMAKE_SOURCE_DIR}/src
)

# Dependencies
find_package(Boost REQUIRED COMPONENTS system thread)

find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(TradingBot 
  ${Boost_LIBRARIES}
  OpenSSL::SSL 
  OpenSSL::Crypto
  nlohmann_json::nlohmann_json
)

