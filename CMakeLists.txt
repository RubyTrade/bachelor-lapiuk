cmake_minimum_required(VERSION 3.16)
project(TradingBot VERSION 1.0)

# CPP Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Sources
file(GLOB_RECURSE SRC_FILES src/*.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(TradingBot ${SRC_FILES})

# Dependencies
find_package(Boost REQUIRED COMPONENTS system thread)

find_package(OpenSSL REQUIRED)

target_link_directories(TradingBot PRIVATE ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

target_link_libraries(TradingBot ${Boost_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)


