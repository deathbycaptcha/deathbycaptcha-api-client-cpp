cmake_minimum_required(VERSION 3.21)

add_executable(test_api_basic test_api_basic.cpp)
target_link_libraries(test_api_basic PRIVATE
    deathbycaptcha_cpp_static GTest::gtest_main)
target_include_directories(test_api_basic PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
add_test(NAME api_basic COMMAND test_api_basic
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
