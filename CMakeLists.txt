cmake_minimum_required(VERSION 3.16)
project(ar_engine)

set(CMAKE_CXX_STANDARD 17)

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
add_subdirectory(external/assimp-5.0.1)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -I/home/magnus/libraries/vulkan/1.2.148.0/x86_64/include -L/home/magnus/libraries/vulkan/1.2.148.0/x86_64/lib -lvulkan")

add_library(static_vulkan src/source/InitiateVulkan.cpp src/headers/InitiateVulkan.h)
target_link_libraries(static_vulkan
        glfw ${GLFW_LIBRARIES}
        vulkan
        )

add_library(mesh src/source/Mesh.cpp src/headers/Mesh.hpp)

add_library(VulkanRenderer src/source/VulkanRenderer.cpp src/headers/VulkanRenderer.hpp)

target_link_libraries(VulkanRenderer
        glfw ${GLFW_LIBRARIES}
        vulkan
        mesh
        assimp
        static_vulkan
        )

add_executable(engine main.cpp src/source/MeshModel.cpp src/headers/MeshModel.h src/headers/PlayerController.h  src/source/Allocator.cpp src/source/Allocator.h)

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        vulkan
        VulkanRenderer
        mesh
        )
