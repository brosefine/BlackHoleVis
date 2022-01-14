cmake_minimum_required(VERSION 3.10)

project(envTest LANGUAGES CXX)

file(GLOB_RECURSE APP_FILES
        ${CMAKE_SOURCE_DIR}/app/envTest/*.cpp
        ${CMAKE_SOURCE_DIR}/app/envTest/*.h)

add_executable(envTest_main ${APP_FILES})
target_include_directories(envTest_main PUBLIC ${CMAKE_SOURCE_DIR}/app/envTest/)
target_link_libraries(envTest_main SOURCE bhv_dependencies)
target_compile_features(envTest_main PRIVATE cxx_std_20)