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


set(LIBRARIES glfw vulkan)
message(LIBRARIES:  ${LIBRARIES})
# DEAR IMGUI LIBRARY
set(IMGUI_DIR external/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)
message(STATUS "Adding GUI library files")
FILE(GLOB GUI_SRC src/renderer/GUI.cpp src/renderer/GUI.h ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp)
add_library(GUI ${GUI_SRC})


file(GLOB ENGINE_SRC src/core/* src/tools/* src/* src/imgui/* src/builder/*)
add_library(VulkanRenderer ${ENGINE_SRC} )
set_target_properties(VulkanRenderer PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(VulkanRenderer GUI ${LIBRARIES})

add_executable(engine main.cpp src/builder/Example.cpp src/builder/Example.h)
target_link_libraries(engine VulkanRenderer)
