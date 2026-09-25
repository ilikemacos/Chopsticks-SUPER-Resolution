cmake_minimum_required(VERSION 3.28)
project(UniversalFrameFX
    VERSION 0.1.0
    DESCRIPTION "Unified Windows 11 interface for FSR/XeSS/Frame Generation"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT WIN32)
    message(WARNING "Universal FrameFX targets Windows 11. Non-Windows builds compile the platform-agnostic core only (for tests / CI cross-check).")
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8 /EHsc)
    add_compile_definitions(UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

option(UFX_BUILD_APP "Build the desktop application (Windows only)" ${WIN32})
option(UFX_BUILD_TESTS "Build the test suite" ON)

include(FetchContent)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)

# stb: public-domain single-header image codecs, for the CSR file upscaler.
# Header-only and not a CMake project, so we expose it as an interface target.
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 5c205738c191bcb0abc65c4febfa9bd25ff35234)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

if(UFX_BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

add_subdirectory(app)

if(UFX_BUILD_TESTS)
    add_subdirectory(tests)
endif()
