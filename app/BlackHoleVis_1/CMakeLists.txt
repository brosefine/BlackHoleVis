cmake_minimum_required(VERSION 3.10)

project(BHVis1 LANGUAGES CXX)

file(GLOB_RECURSE APP1_FILES
        ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_1/*.cpp
        ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_1/*.h)

add_executable(BHVis1_main ${APP1_FILES})
target_include_directories(BHVis1_main PUBLIC ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_1/)
target_link_libraries(BHVis1_main SOURCE bhv_dependencies)
target_compile_features(BHVis1_main PRIVATE cxx_std_20)