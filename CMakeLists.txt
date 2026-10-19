cmake_minimum_required(VERSION 3.21)
project(GDStandaloneSafeClick VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

add_executable(predictor_tests tests/predictor_tests.cpp)
add_executable(controller_tests tests/controller_tests.cpp)
add_executable(live_calibration_tests tests/live_calibration_tests.cpp)
add_test(NAME predictor COMMAND predictor_tests)
add_test(NAME controller COMMAND controller_tests)
add_test(NAME live_calibration COMMAND live_calibration_tests)

if(WIN32)
    add_library(gd_safeclick_proxy SHARED src/Proxy.cpp src/xinput.def)
    target_compile_definitions(gd_safeclick_proxy PRIVATE UNICODE _UNICODE)
    set_target_properties(gd_safeclick_proxy PROPERTIES
        OUTPUT_NAME "XINPUT9_1_0"
        PREFIX ""
    )
endif()
