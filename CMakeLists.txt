cmake_minimum_required(VERSION 3.16)
project(ar_engine)

set(CMAKE_CXX_STANDARD 17)

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenCV)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -I/home/magnus/libraries/vulkan/1.2.148.0/x86_64/include -L/home/magnus/libraries/vulkan/1.2.148.0/x86_64/lib -lvulkan -lOpenCL -L/opt/rocm-3.8.0/opencl/lib/")

## Stereovision extension
add_library(stereoVision src/stereo/Disparity.cpp src/stereo/Disparity.h)
target_link_libraries(stereoVision PUBLIC ${OpenCV_LIBS} ${CMAKE_THREAD_LIBS_INIT} glfw)
target_compile_options(stereoVision PUBLIC -pthread)

## Descriptor Library
add_library(Descriptors
        src/pipeline/Descriptors.cpp
        src/pipeline/Descriptors.h)

## Hardware setup
add_library(hw_platform src/Platform/Platform.cpp src/Platform/Platform.h)

# Graphics pipeline
add_library(pipeline src/pipeline/Pipeline.cpp src/pipeline/Pipeline.h)

#Object loading library Meshes
add_library(mesh src/pipeline/Mesh.cpp src/pipeline/Mesh.h)

# Ties vulkanRenderer with helper classes
add_library(VulkanRenderer
        src/renderer/VulkanRenderer.cpp
        src/renderer/VulkanRenderer.hpp
        src/include/Validation.h
        src/include/BufferCreation.h
        )
target_link_libraries(VulkanRenderer
        vulkan
        Descriptors
        hw_platform
        pipeline
        mesh
        )

add_library(GameApplication src/renderer/GameApplication.cpp src/renderer/GameApplication.h src/include/Camera.h)
target_link_libraries(GameApplication
        VulkanRenderer
        stereoVision
        )

add_executable(engine main.cpp src/include/structs.h)

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        VulkanRenderer
        GameApplication
        )
