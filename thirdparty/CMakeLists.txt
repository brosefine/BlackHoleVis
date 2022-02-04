cmake_minimum_required(VERSION 3.10)

project(bhv_dependencies LANGUAGES CXX)

if(CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_TOOLCHAIN_FILE})
endif(CMAKE_TOOLCHAIN_FILE)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb.h")
find_package(Boost REQUIRED COMPONENTS json)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${STB_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} INTERFACE glad::glad glm::glm glfw imgui::imgui implot::implot Boost::boost Boost::json tinyobjloader::tinyobjloader)
target_link_libraries(${PROJECT_NAME} INTERFACE glad::glad glm::glm glfw imgui::imgui implot::implot Boost::boost Boost::json tinyobjloader::tinyobjloader cereal)