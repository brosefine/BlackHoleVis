cmake_minimum_required(VERSION 3.10)

project(BHVis2 LANGUAGES CXX)

file(GLOB_RECURSE APP_FILES
        ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_2/*.cpp
        ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_2/*.h)

add_executable(BHVis2_main ${APP_FILES})
target_include_directories(BHVis2_main PUBLIC ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_2/)
target_link_libraries(BHVis2_main SOURCE bhv_dependencies)
target_compile_features(BHVis2_main PRIVATE cxx_std_20)