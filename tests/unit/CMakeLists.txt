cmake_minimum_required(VERSION 3.21)

# Unit tests — compiled against the static library
set(DBC_TEST_LIB deathbycaptcha_cpp_static)

# ── test_models ────────────────────────────────────────────────────────────────
add_executable(test_models test_models.cpp)
target_link_libraries(test_models PRIVATE ${DBC_TEST_LIB} GTest::gtest_main)
target_include_directories(test_models PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME models COMMAND test_models)

# ── test_exceptions ────────────────────────────────────────────────────────────
add_executable(test_exceptions test_exceptions.cpp)
target_link_libraries(test_exceptions PRIVATE ${DBC_TEST_LIB} GTest::gtest_main)
target_include_directories(test_exceptions PRIVATE
    ${CMAKE_SOURCE_DIR}/include)
add_test(NAME exceptions COMMAND test_exceptions)

# ── test_client_core ───────────────────────────────────────────────────────────
add_executable(test_client_core test_client_core.cpp)
target_link_libraries(test_client_core PRIVATE ${DBC_TEST_LIB}
    GTest::gtest_main GTest::gmock)
target_include_directories(test_client_core PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME client_core COMMAND test_client_core)

# ── test_http_client ───────────────────────────────────────────────────────────
add_executable(test_http_client test_http_client.cpp)
target_link_libraries(test_http_client PRIVATE ${DBC_TEST_LIB}
    GTest::gtest_main GTest::gmock)
target_include_directories(test_http_client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME http_client COMMAND test_http_client)

# ── test_socket_client ─────────────────────────────────────────────────────────
add_executable(test_socket_client test_socket_client.cpp)
target_link_libraries(test_socket_client PRIVATE ${DBC_TEST_LIB}
    GTest::gtest_main GTest::gmock)
target_include_directories(test_socket_client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME socket_client COMMAND test_socket_client)

# ── test_coverage_boost ────────────────────────────────────────────────────────
add_executable(test_coverage_boost test_coverage_boost.cpp)
target_link_libraries(test_coverage_boost PRIVATE ${DBC_TEST_LIB}
    GTest::gtest_main GTest::gmock)
target_include_directories(test_coverage_boost PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME coverage_boost COMMAND test_coverage_boost)
