cmake_minimum_required(VERSION 3.16)
project(ar_engine)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17  -L/opt/amdgpu-pro/lib/x86_64-linux-gnu -lamdocl-orca64")


find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenCV REQUIRED)

find_package(OpenCL REQUIRED)

MESSAGE("Vulkan lib ${Vulkan_LIBS}")

## Stereovision extension
add_library(stereoVision src/stereo/Disparity.cpp src/stereo/Disparity.h)
target_link_libraries(stereoVision PUBLIC ${OpenCV_LIBS}  )
target_compile_options(stereoVision PUBLIC -pthread )

# Ties vulkanRenderer with helper classes
add_library(VulkanRenderer
        src/renderer/VulkanRenderer.cpp
        src/renderer/VulkanRenderer.hpp
        src/include/Validation.h
        src/pipeline/Buffer.cpp
        src/pipeline/Buffer.h
        src/pipeline/Pipeline.cpp
        src/pipeline/Pipeline.h
        src/Platform/Platform.cpp
        src/Platform/Platform.h
        src/pipeline/Mesh.cpp
        src/pipeline/Mesh.h
        src/pipeline/Descriptors.cpp
        src/pipeline/Descriptors.h
        src/renderer/GameApplication.cpp
        src/renderer/GameApplication.h
        src/include/structs.h
        src/pipeline/Camera.cpp
        src/pipeline/Camera.h)

target_link_libraries(VulkanRenderer
        stereoVision
        vulkan
        )


add_executable(engine main.cpp)

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        VulkanRenderer
        )


add_executable(test test.cpp)
#target_link_libraries(test )


