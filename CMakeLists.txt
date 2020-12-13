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
  src/Shader.hpp
  src/Shader.cpp)

set_property(TARGET main PROPERTY CXX_STANDARD 17)
target_compile_options(main PRIVATE -Wall)

# glfw
add_subdirectory(lib/glfw EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE glfw)

# glew
add_definitions(-DGLEW_STATIC)
add_subdirectory(lib/glew EXCLUDE_FROM_ALL)
target_link_libraries(main PRIVATE libglew_static)

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

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)