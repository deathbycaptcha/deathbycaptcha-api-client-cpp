cmake_minimum_required(VERSION 3.21)

project(deathbycaptcha-cpp VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(FetchContent)

# ── Options ─────────────────────────────────────────────────────────────────
option(DBC_BUILD_SHARED           "Build the shared library"                    ON)
option(DBC_BUILD_STATIC           "Build the static library"                    ON)
option(DBC_BUILD_TESTS            "Build unit tests"                            ON)
option(DBC_BUILD_INTEGRATION_TESTS "Build live integration tests (need creds)"  OFF)
option(DBC_BUILD_EXAMPLES         "Build example executables"                   OFF)
option(DBC_ENABLE_COVERAGE        "Enable gcov/llvm-cov coverage flags"         OFF)

# ── C++ standard ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# ── External dependencies ────────────────────────────────────────────────────
# libcurl  (system)
find_package(CURL REQUIRED)

# nlohmann/json  (header-only — FetchContent or system)
find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ── Warning interface ─────────────────────────────────────────────────────────
add_library(dbc_cpp_warnings INTERFACE)
if(MSVC)
    target_compile_options(dbc_cpp_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(dbc_cpp_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wnon-virtual-dtor)
endif()

if(DBC_ENABLE_COVERAGE AND NOT MSVC)
    target_compile_options(dbc_cpp_warnings INTERFACE --coverage -O0 -g)
    target_link_options(dbc_cpp_warnings INTERFACE --coverage)
endif()

# ── Platform libs ─────────────────────────────────────────────────────────────
find_package(Threads REQUIRED)
set(DBC_PLATFORM_LIBS Threads::Threads CURL::libcurl nlohmann_json::nlohmann_json)
if(WIN32)
    list(APPEND DBC_PLATFORM_LIBS ws2_32)
endif()

# ── Source list ───────────────────────────────────────────────────────────────
set(DBC_SOURCES
    src/deathbycaptcha.cpp
)
set(DBC_PUBLIC_HEADERS
    include/deathbycaptcha/deathbycaptcha.hpp
)

# ── Helper to apply common settings ───────────────────────────────────────────
function(dbc_configure_target target_name)
    target_link_libraries(${target_name}
        PRIVATE dbc_cpp_warnings ${DBC_PLATFORM_LIBS})
    target_include_directories(${target_name}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(${target_name} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        OUTPUT_NAME deathbycaptcha_cpp)
endfunction()

# ── Shared library ────────────────────────────────────────────────────────────
if(DBC_BUILD_SHARED)
    add_library(deathbycaptcha_cpp_shared SHARED ${DBC_SOURCES})
    dbc_configure_target(deathbycaptcha_cpp_shared)
    target_compile_definitions(deathbycaptcha_cpp_shared PRIVATE DBC_BUILDING_DLL=1)
    set_target_properties(deathbycaptcha_cpp_shared PROPERTIES
        VERSION   ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# ── Static library ────────────────────────────────────────────────────────────
if(DBC_BUILD_STATIC)
    add_library(deathbycaptcha_cpp_static STATIC ${DBC_SOURCES})
    dbc_configure_target(deathbycaptcha_cpp_static)
    set_target_properties(deathbycaptcha_cpp_static PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# ── Alias for consumers ───────────────────────────────────────────────────────
if(DBC_BUILD_STATIC)
    add_library(DeathByCaptcha::cpp ALIAS deathbycaptcha_cpp_static)
elseif(DBC_BUILD_SHARED)
    add_library(DeathByCaptcha::cpp ALIAS deathbycaptcha_cpp_shared)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
if(DBC_BUILD_TESTS OR DBC_BUILD_INTEGRATION_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/releases/download/v1.14.0/googletest-1.14.0.tar.gz
            URL_HASH SHA256=8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    enable_testing()
endif()

if(DBC_BUILD_TESTS)
    add_subdirectory(tests/unit)
endif()

if(DBC_BUILD_INTEGRATION_TESTS)
    add_subdirectory(tests/integration)
endif()

# ── Examples ──────────────────────────────────────────────────────────────────
if(DBC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(FILES ${DBC_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/deathbycaptcha)

if(DBC_BUILD_SHARED)
    install(TARGETS deathbycaptcha_cpp_shared
        EXPORT DeathByCaptchaCppTargets
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(DBC_BUILD_STATIC)
    install(TARGETS deathbycaptcha_cpp_static
        EXPORT DeathByCaptchaCppTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
