cmake_minimum_required(VERSION 3.10)

project(KerrVis LANGUAGES CXX)

file(GLOB_RECURSE APP_FILES
        ${CMAKE_SOURCE_DIR}/app/KerrVis/*.cpp
        ${CMAKE_SOURCE_DIR}/app/KerrVis/*.h)

add_executable(KerrVis_main ${APP_FILES})
target_include_directories(KerrVis_main PUBLIC ${CMAKE_SOURCE_DIR}/app/KerrVis/)
target_link_libraries(KerrVis_main SOURCE bhv_dependencies)
target_compile_features(KerrVis_main PRIVATE cxx_std_20)