cmake_minimum_required(VERSION 3.10)
project(ar_engine)

set(CMAKE_CXX_STANDARD 17)

# Include Vulkan and OpenCL from AMD drivers
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17  -pthread")#-L/opt/amdgpu-pro/lib/x86_64-linux-gnu -lamdocl-orca64")

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenCV REQUIRED)

## JSON Parser
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(external/json)

add_library(videoStream src/record/ThreadSpawner.cpp src/record/ThreadSpawner.h)

target_link_libraries(videoStream ${OpenCV_LIBS} -lv4l2)

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
        src/pipeline/Camera.h
        src/pipeline/Images.cpp
        src/pipeline/Images.h
        src/pipeline/Textures.cpp
        src/pipeline/Textures.h
        src/include/stb_image.h
        src/include/stbi_image_write.h
        src/pipeline/MeshModel.cpp
        src/pipeline/MeshModel.h
        src/Platform/LoadSettings.cpp
        src/Platform/LoadSettings.h src/renderer/VulkanCompute.cpp src/renderer/VulkanCompute.h src/objects/BufferObject.cpp src/objects/BufferObject.h src/record/ThreadSpawner.cpp src/record/ThreadSpawner.h)

target_link_libraries(VulkanRenderer
        vulkan
        nlohmann_json::nlohmann_json
        videoStream
        )

add_executable(engine main.cpp external/tinyobj/tiny_obj_loader.h)

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        VulkanRenderer
        nlohmann_json::nlohmann_json
        )


