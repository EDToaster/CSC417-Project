# Projects Settings
cmake_minimum_required (VERSION 3.1)
project (RecreatingNoita)

# The main executable
add_executable(main
  src/glError.hpp
  src/glError.cpp
  src/main.cpp
  src/UI.hpp
  src/Types.hpp
  src/Simulation.hpp
  src/Marching.hpp
  # TODO: Phase 2 - Re-add shader files when migrating to Vulkan shaders
  # src/Shader.hpp
  # src/Shader.cpp
  src/polypartition.cpp
  src/polypartition.h
  src/VulkanContext.hpp
  src/VulkanContext.cpp
  src/VulkanSwapchain.hpp
  src/VulkanSwapchain.cpp
  )

set_property(TARGET main PROPERTY CXX_STANDARD 17)
target_compile_options(main PRIVATE -Wall)

# glfw
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE glfw)

# Vulkan
# Try to find Vulkan SDK
# On Windows, VULKAN_SDK environment variable should be set
# On Linux, Vulkan is typically in system paths
if(DEFINED ENV{VULKAN_SDK})
    message(STATUS "Found Vulkan SDK via VULKAN_SDK environment variable: $ENV{VULKAN_SDK}")
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    
    # Set include directory
    set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_PATH}/Include")
    if(NOT EXISTS "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.h")
        message(FATAL_ERROR "Vulkan header not found at ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.h")
    endif()
    
    # Set library path based on platform
    if(WIN32)
        set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib")
        if(NOT EXISTS "${Vulkan_LIBRARY}")
            message(FATAL_ERROR "Vulkan library not found at ${Vulkan_LIBRARY}")
        endif()
    else()
        # Linux - try common locations
        if(EXISTS "${VULKAN_SDK_PATH}/lib/x86_64-linux-gnu/libvulkan.so")
            set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/lib/x86_64-linux-gnu/libvulkan.so")
        elseif(EXISTS "${VULKAN_SDK_PATH}/lib/libvulkan.so")
            set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/lib/libvulkan.so")
        else()
            message(FATAL_ERROR "Vulkan library not found in ${VULKAN_SDK_PATH}/lib")
        endif()
    endif()
    
    message(STATUS "Vulkan Include: ${Vulkan_INCLUDE_DIR}")
    message(STATUS "Vulkan Library: ${Vulkan_LIBRARY}")
    
    target_include_directories(main PRIVATE ${Vulkan_INCLUDE_DIR})
    target_link_libraries(main PRIVATE ${Vulkan_LIBRARY})
else()
    # Try to find Vulkan using CMake's find_package
    find_package(Vulkan)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR 
            "Vulkan SDK not found!\n"
            "Please install the Vulkan SDK from https://vulkan.lunarg.com/\n"
            "On Windows, set the VULKAN_SDK environment variable to the SDK installation path.\n"
            "Example: C:/VulkanSDK/1.3.xxx.x\n"
            "After installation, restart your terminal/IDE for the environment variable to take effect.\n"
            "See VULKAN_SETUP.md for detailed instructions."
        )
    else()
        target_include_directories(main PRIVATE ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(main PRIVATE ${Vulkan_LIBRARIES})
    endif()
endif()

# glew (kept for now during migration, will be removed later)
# add_definitions(-DGLEW_STATIC)
# add_subdirectory(lib/glew EXCLUDE_FROM_ALL)
# target_link_libraries(main PRIVATE libglew_static)

# glm
add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE glm)

# freetype2
add_subdirectory(lib/freetype2 EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE freetype)

# box2d
option(BOX2D_BUILD_TESTBED "Build the Box2D testbed" OFF)
add_subdirectory(lib/box2d EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE box2d)

# openmp
find_package(OpenMP REQUIRED)
target_link_libraries(main PRIVATE OpenMP::OpenMP_CXX)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)