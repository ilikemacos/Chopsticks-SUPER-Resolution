add_executable(ufx_tests
    test_gpu.cpp
    test_profiles.cpp
    test_backup.cpp
    test_scanner.cpp
    test_upscalers.cpp
    test_framegen.cpp)

target_link_libraries(ufx_tests PRIVATE ufx_core GTest::gtest_main)
target_compile_definitions(ufx_tests PRIVATE
    UFX_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/examples/profiles")

include(GoogleTest)
gtest_discover_tests(ufx_tests)
