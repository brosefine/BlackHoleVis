cmake_minimum_required(VERSION 3.10)

project(KerrVis LANGUAGES CXX)

file(GLOB APP_FILES
        ${CMAKE_SOURCE_DIR}/app/KerrVis/kerr_app.cpp
        ${CMAKE_SOURCE_DIR}/app/KerrVis/kerr_main.cpp
        ${CMAKE_SOURCE_DIR}/app/KerrVis/kerr_app.h)



add_executable(KerrVis_main ${APP_FILES})
target_include_directories(KerrVis_main PUBLIC ${CMAKE_SOURCE_DIR}/app/KerrVis/)
target_link_libraries(KerrVis_main SOURCE bhv_dependencies)
target_compile_features(KerrVis_main PRIVATE cxx_std_20)

# simple hello world application for testing
#file(GLOB HELLO_FILES
#         ${CMAKE_SOURCE_DIR}/app/KerrVis/main/hello.cpp)
#add_executable(hello_main ${HELLO_FILES})
#target_link_libraries(hello_main SOURCE bhv_dependencies)
