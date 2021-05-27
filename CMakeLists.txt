cmake_minimum_required(VERSION 3.10)

project(BlackHoleVis VERSION 1.0 LANGUAGES CXX)

add_subdirectory(thirdparty)

file(GLOB SRC_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp)

file(GLOB HEADER_FILES
        ${CMAKE_SOURCE_DIR}/include/*.hpp)

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SRC_FILES})