cmake_minimum_required(VERSION 3.10)
project(ar_engine)

set(CMAKE_CXX_STANDARD 17)

# Include Vulkan and OpenCL from AMD drivers
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_BUILD_TYPE Debug)

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenCV REQUIRED)

## JSON Parser
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(external/json)

#DLIB
add_subdirectory($ENV{HOME}/libraries/dlib dlib_build)
# If you have cmake 3.14 or newer you can even use FetchContent instead of
# add_subdirectory() to pull in dlib as a dependency.  So instead of using the
# above add_subdirectory() command, you could use the following three commands
# to make dlib available:
#include(FetchContent)
#FetchContent_Declare(dlib
#       GIT_REPOSITORY https://github.com/davisking/dlib.git
#       GIT_TAG        v19.18
#)
#FetchContent_MakeAvailable(dlib)


add_library(videoStream
        src/FaceAugment/ThreadSpawner.cpp
        src/FaceAugment/ThreadSpawner.h
        src/FaceAugment/RealsenseStreamer.cpp
        src/FaceAugment/RealsenseStreamer.h
        src/FaceAugment/FaceDetector.cpp
        src/FaceAugment/FaceDetector.h)

target_link_libraries(videoStream
        ${OpenCV_LIBS}
        -lv4l2
        -lrealsense2
        dlib::dlib
        )


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
        src/Models/Mesh.cpp
        src/Models/Mesh.h
        src/pipeline/Descriptors.cpp
        src/pipeline/Descriptors.h
        src/include/structs.h
        src/Platform/Camera.cpp
        src/Platform/Camera.h
        src/pipeline/Images.cpp
        src/pipeline/Images.h
        src/pipeline/Textures.cpp
        src/pipeline/Textures.h
        src/include/stb_image.h
        src/include/stbi_image_write.h
        src/Models/MeshModel.cpp
        src/Models/MeshModel.h
        src/Platform/LoadSettings.cpp
        src/Platform/LoadSettings.h
        src/renderer/VulkanCompute.cpp
        src/renderer/VulkanCompute.h
        src/include/helper_functions.h
        src/Models/SceneObject.cpp
        src/Models/SceneObject.h
        )

target_link_libraries(VulkanRenderer
        vulkan
        nlohmann_json::nlohmann_json
        videoStream
        ${OpenCV_LIBS}
        )

add_executable(engine main.cpp
        external/tinyobj/tiny_obj_loader.h
        src/renderer/GameApplication.cpp
        src/renderer/GameApplication.h)

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        VulkanRenderer
        nlohmann_json::nlohmann_json
        )


