cmake_minimum_required(VERSION 3.10)

project(BHVis3 LANGUAGES CXX)

file(GLOB_RECURSE APP_FILES
        ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_3/*.cpp
        ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_3/*.h)

add_executable(BHVis3_main ${APP_FILES})
target_include_directories(BHVis3_main PUBLIC ${CMAKE_SOURCE_DIR}/app/BlackHoleVis_3/)
target_link_libraries(BHVis3_main SOURCE bhv_dependencies)
target_compile_features(BHVis3_main PRIVATE cxx_std_20)