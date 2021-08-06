cmake_minimum_required(VERSION 3.10)
project(ar_engine)

set(CMAKE_CXX_STANDARD 17)

# Include Vulkan and OpenCL from AMD drivers
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -I/usr/local/include/opencv4 -I/home/magnus/libraries/dlib/")
set(CMAKE_BUILD_TYPE Debug)

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenCV 4.0.0 REQUIRED)

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

message(STATUS "Adding camera library source files")
FILE(GLOB VIDEO_STREAM_SRC src/FaceAugment/*.cpp src/FaceAugment/*.h)
add_library(videoStream ${VIDEO_STREAM_SRC})

target_link_libraries(videoStream
        -lv4l2
        ${OpenCV_LIBS}
        -lrealsense2
        dlib::dlib
        )


# Ties vulkanRenderer with helper classes
message(STATUS "Adding platform files")
FILE(GLOB PLATFORM_SRC src/Platform/*.cpp src/Platform/*.h)
add_library(Platform ${PLATFORM_SRC})
target_link_libraries(Platform nlohmann_json::nlohmann_json)

message(STATUS "Adding pipeline files")
FILE(GLOB PIPELINE_SRC src/pipeline/*.cpp src/pipeline/*.h)
add_library(Pipeline ${PIPELINE_SRC})

message(STATUS "Adding model loading files")
FILE(GLOB MODEL_SRC src/Models/*.cpp src/Models/*.h)
add_library(Models ${MODEL_SRC})
target_link_libraries(Models nlohmann_json::nlohmann_json)

message(STATUS "Adding GUI library files")
FILE(GLOB GUI_SRC src/renderer/GUI.cpp src/renderer/GUI.h)
add_library(GUI ${GUI_SRC})
target_link_libraries(GUI)

add_library(VulkanRenderer
        src/renderer/VulkanCompute.cpp
        src/renderer/VulkanCompute.h
        src/renderer/VulkanRenderer.cpp
        src/renderer/VulkanRenderer.hpp
        src/include/Validation.h
        src/include/structs.h
        src/include/stb_image.h
        src/include/stbi_image_write.h
        src/include/helper_functions.h
        )

target_link_libraries(VulkanRenderer
        vulkan
        videoStream
        -lnoise
        Platform
        Pipeline
        Models
        GUI
        )

add_executable(engine main.cpp
        external/tinyobj/tiny_obj_loader.h
        src/renderer/GameApplication.cpp
        src/renderer/GameApplication.h)

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        VulkanRenderer
        )


