cmake_minimum_required(VERSION 3.16)
project(ar_engine)

set(CMAKE_CXX_STANDARD 17)

find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -I/home/magnus/libraries/vulkan/1.2.148.0/x86_64/include -L/home/magnus/libraries/vulkan/1.2.148.0/x86_64/lib -lvulkan")


add_library(VulkanRenderer src/source/VulkanRenderer.cpp src/headers/VulkanRenderer.hpp src/headers/validation.h)
target_link_libraries(VulkanRenderer

        vulkan
        )

add_library(GameApplication src/source/GameApplication.cpp src/source/GameApplication.h)
target_link_libraries(GameApplication
        VulkanRenderer
        )

add_executable(engine main.cpp)

target_link_libraries(engine
        glfw ${GLFW_LIBRARIES}
        VulkanRenderer
        GameApplication
        )
