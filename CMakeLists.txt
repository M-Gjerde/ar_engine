cmake_minimum_required(VERSION 3.16)
project(udemy_vulcan)

set(CMAKE_CXX_STANDARD 20)

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
add_subdirectory(external/glfw-3.3.2)

add_executable(engine main.cpp)

add_library(VulkanRenderer src/source/VulkanRenderer.cpp src/headers/VulkanRenderer.hpp)
add_library(mesh src/source/Mesh.cpp src/headers/Mesh.hpp)

target_link_libraries(VulkanRenderer
        glfw ${GLFW_LIBRARIES}
        vulkan
        mesh
        )

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        vulkan
        VulkanRenderer
        mesh
        )
