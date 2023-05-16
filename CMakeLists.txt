cmake_minimum_required(VERSION 3.10)

project(fixer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenSSL REQUIRED)

add_executable(main main.cpp)

target_link_libraries(main OpenSSL::Crypto)
