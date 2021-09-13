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

# Perlin Noise Library
message(STATUS "Adding Perlin noise files")
FILE(GLOB PERLIN_SRC external/Perlin_Noise/*)
add_library(Perlin ${PERLIN_SRC})

# DEAR IMGUI LIBRARY
set(IMGUI_DIR external/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)
message(STATUS "Adding GUI library files")
FILE(GLOB GUI_SRC src/renderer/GUI.cpp src/renderer/GUI.h ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
add_library(GUI ${GUI_SRC})

# tiny-glTF header only loader
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
include_directories(external/tinygltf/)

# VULKAN KTX TOOLS
set(KTX_DIR external/KTX-Software)
add_subdirectory(${KTX_DIR})

# Create a list of Libraries in project
set(LIBRARIES glfw vulkan Perlin ktx)
message(LIBRARIES:  ${LIBRARIES})

file(GLOB ENGINE_SRC src/core/* src/tools/* src/* src/imgui/* src/builder/*)
add_library(VulkanRenderer ${ENGINE_SRC} src/core/VulkanglTFModel.cpp.unfinished src/core/VulkanglTFModel.h.unfinished)
set_target_properties(VulkanRenderer PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(VulkanRenderer GUI ${LIBRARIES})

add_executable(engine main.cpp src/core/structs.h)
target_link_libraries(engine VulkanRenderer)
