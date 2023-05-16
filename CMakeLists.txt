cmake_minimum_required(VERSION 3.10)

project(fixer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenSSL REQUIRED)

add_executable(main src/main.cpp src/FixSession.cpp src/LogonMessage.cpp)
target_link_libraries(main OpenSSL::Crypto)

# include GTest
add_subdirectory(external/googletest)
enable_testing()

# Create a new executable target for tests
add_executable(tests test_main.cpp src/FixSession.cpp src/LogonMessage.cpp)
target_link_libraries(tests gtest_main OpenSSL::Crypto)
add_test(NAME fixer_tests COMMAND tests)