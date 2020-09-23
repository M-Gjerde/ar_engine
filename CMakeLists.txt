cmake_minimum_required(VERSION 3.16)
project(ar_engine)

set(CMAKE_CXX_STANDARD 17)

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
add_subdirectory(external/assimp-5.0.1)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -I/home/magnus/libraries/vulkan/1.2.148.0/x86_64/include -L/home/magnus/libraries/vulkan/1.2.148.0/x86_64/lib -lvulkan")

add_library(static_vulkan src/Mesh/Object.h src/source/Vfd.cpp src/headers/Vfd.h src/headers/Camera.h)
target_link_libraries(static_vulkan
        glfw ${GLFW_LIBRARIES}
        vulkan
        )

add_library(dynamic_vulkan src/libs/Utils.cpp src/libs/TextureLoading.cpp src/libs/Utils.h src/libs/TextureLoading.h src/libs/GraphicsPipeline.cpp src/libs/GraphicsPipeline.h)
target_link_libraries(dynamic_vulkan
        glfw ${GLFW_LIBRARIES}
        vulkan
        )

add_library(mesh src/Mesh/Mesh.cpp src/Mesh/Mesh.hpp)

add_library(VulkanRenderer src/source/VulkanRenderer.cpp src/headers/VulkanRenderer.hpp)
target_link_libraries(VulkanRenderer
        glfw ${GLFW_LIBRARIES}
        vulkan
        mesh
        static_vulkan
        dynamic_vulkan
        assimp
        )

add_library(GameApplication src/source/GameApplication.cpp src/source/GameApplication.h)
target_link_libraries(GameApplication
        VulkanRenderer
        )

add_executable(engine main.cpp src/Mesh/MeshModel.cpp src/Mesh/MeshModel.h src/source/Allocator.cpp src/source/Allocator.h)

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        vulkan
        VulkanRenderer
        mesh
        assimp
        GameApplication
        )
