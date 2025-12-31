# Vulkan Migration Plan: OpenGL to Vulkan Renderer

## Project Overview
This project is a particle physics simulation (similar to Noita) that currently uses OpenGL for rendering. The migration to Vulkan will require significant architectural changes due to Vulkan's explicit, low-level API design.

## Current OpenGL Implementation Analysis

### Key OpenGL Features Used
1. **Immediate Mode Rendering**: `glBegin/glEnd`, `glRectf`, `glVertex2f`, `glColor3f`
2. **Framebuffers**: Render-to-texture for offscreen rendering
3. **Shader Storage Buffer Objects (SSBOs)**: Particle data storage
4. **Fragment Shaders**: Particle rendering, screen compositing, rigid body rendering
5. **Texture Management**: 2D textures for framebuffer attachments
6. **UI Rendering**: Immediate mode for circles, rectangles, and text
7. **FreeType Integration**: Text rendering via bitmap fonts

### Current Rendering Pipeline
1. Initialize OpenGL context via GLFW + GLEW
2. Create offscreen framebuffer with texture attachment
3. Create SSBO for particle data
4. Each frame:
   - Update particle data in SSBO
   - Render to offscreen framebuffer using base shader
   - Optionally render rigid bodies using rigid shader
   - Blit framebuffer to screen using screen shader
   - Render UI elements using immediate mode

## Migration Strategy

### Phase 1: Infrastructure Setup

#### 1.1 Replace GLEW with Vulkan Loader
- **Action**: Remove GLEW dependency, add Vulkan SDK
- **Files**: `CMakeLists.txt`, `src/main.cpp`
- **Details**:
  - Use Vulkan Loader (vulkan.h) instead of GLEW
  - Initialize Vulkan instance
  - Select physical device
  - Create logical device
  - Set up validation layers (for debugging)

#### 1.2 Replace GLFW OpenGL Context with Vulkan Surface
- **Action**: Modify window creation to use Vulkan surface
- **Files**: `src/main.cpp` (InitializeAndCreateWindow function)
- **Details**:
  - Use `glfwCreateWindowSurface()` instead of `glfwMakeContextCurrent()`
  - Remove GLEW initialization
  - Set up swapchain for presentation

#### 1.3 Create Vulkan Wrapper Classes
- **Action**: Create abstraction layer for Vulkan objects
- **New Files**:
  - `src/VulkanContext.hpp/cpp`: Instance, device, queues management
  - `src/VulkanSwapchain.hpp/cpp`: Swapchain and image management
  - `src/VulkanCommandBuffer.hpp/cpp`: Command buffer management
  - `src/VulkanBuffer.hpp/cpp`: Buffer management (replaces SSBO)
  - `src/VulkanTexture.hpp/cpp`: Texture/image management
  - `src/VulkanFramebuffer.hpp/cpp`: Framebuffer management
  - `src/VulkanShader.hpp/cpp`: Shader module management (replaces Shader.hpp/cpp)
  - `src/VulkanPipeline.hpp/cpp`: Pipeline state objects
  - `src/VulkanRenderer.hpp/cpp`: High-level rendering interface

### Phase 2: Core Rendering Migration

#### 2.1 Replace Immediate Mode with Vertex Buffers
- **Action**: Convert all immediate mode rendering to vertex buffer-based rendering
- **Files**: `src/main.cpp`, `src/UI.hpp`
- **Details**:
  - Create vertex buffers for fullscreen quad (replaces `glRectf`)
  - Create vertex buffers for UI elements (circles, rectangles)
  - Remove all `glBegin/glEnd` calls
  - Use indexed or non-indexed vertex drawing

#### 2.2 Migrate Shader System
- **Action**: Convert GLSL shaders to Vulkan-compatible format
- **Files**: `shader/*.frag`, `src/Shader.hpp/cpp` → `src/VulkanShader.hpp/cpp`
- **Details**:
  - Convert shaders to use Vulkan GLSL (version 450)
  - Update descriptor set layouts (replaces uniform locations)
  - Compile shaders to SPIR-V using `glslc` or runtime compilation
  - Create shader modules from SPIR-V

#### 2.3 Replace SSBO with Vulkan Storage Buffer
- **Action**: Migrate particle data storage
- **Files**: `src/main.cpp`
- **Details**:
  - Create Vulkan buffer with `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT`
  - Use descriptor sets to bind buffer to shaders
  - Update buffer data using staging buffer or direct mapping
  - Ensure proper memory barriers for GPU access

#### 2.4 Migrate Framebuffer System
- **Action**: Replace OpenGL framebuffers with Vulkan render passes
- **Files**: `src/main.cpp`
- **Details**:
  - Create render pass for offscreen rendering
  - Create image and image view for render target
  - Create framebuffer for render pass
  - Use render pass begin/end commands instead of `glBindFramebuffer()`

### Phase 3: Pipeline and Rendering

#### 3.1 Create Graphics Pipelines
- **Action**: Define pipeline state objects for each shader
- **New Files**: `src/VulkanPipeline.hpp/cpp`
- **Details**:
  - Base particle rendering pipeline
  - Rigid body rendering pipeline
  - Screen compositing pipeline
  - UI rendering pipeline
  - Define vertex input, rasterization, multisampling, depth/stencil, color blending states

#### 3.2 Implement Command Recording
- **Action**: Replace immediate OpenGL calls with command buffer recording
- **Files**: `src/main.cpp`, `src/VulkanRenderer.hpp/cpp`
- **Details**:
  - Record commands for each frame
  - Bind pipelines, descriptor sets, vertex buffers
  - Draw calls using `vkCmdDraw` or `vkCmdDrawIndexed`
  - Synchronization with semaphores and fences

#### 3.3 Implement Synchronization
- **Action**: Add proper synchronization primitives
- **Files**: `src/VulkanContext.hpp/cpp`
- **Details**:
  - Semaphores for swapchain image acquisition and rendering completion
  - Fences for CPU-GPU synchronization
  - Memory barriers for buffer/image access
  - Ensure proper ordering of operations

### Phase 4: UI Rendering Migration

#### 4.1 Migrate Circle Drawing
- **Action**: Replace immediate mode circle with vertex buffer
- **Files**: `src/UI.hpp`
- **Details**:
  - Pre-generate circle vertices (already partially done)
  - Create vertex buffer for circle geometry
  - Use instancing for multiple circles if needed
  - Create separate pipeline for UI rendering

#### 4.2 Migrate Rectangle Drawing
- **Action**: Replace `glRectf` with vertex buffer
- **Files**: `src/UI.hpp`
- **Details**:
  - Create vertex buffer for rectangle quads
  - Use indexed drawing for efficiency
  - Support dynamic rectangle updates

#### 4.3 Migrate Text Rendering
- **Action**: Convert FreeType bitmap rendering to Vulkan texture
- **Files**: `src/UI.hpp`
- **Details**:
  - Create texture atlas from FreeType glyphs
  - Upload glyph data to Vulkan image
  - Render text using textured quads
  - Create descriptor set for font texture

### Phase 5: Advanced Features

#### 5.1 Memory Management
- **Action**: Implement proper memory allocation strategy
- **Files**: `src/VulkanBuffer.hpp/cpp`, `src/VulkanTexture.hpp/cpp`
- **Details**:
  - Use Vulkan Memory Allocator (VMA) library for easier memory management
  - Or implement custom allocator for device memory
  - Handle buffer/image memory requirements
  - Implement staging buffers for CPU-to-GPU transfers

#### 5.2 Descriptor Set Management
- **Action**: Create descriptor pool and sets
- **Files**: `src/VulkanContext.hpp/cpp`
- **Details**:
  - Create descriptor pool with appropriate sizes
  - Allocate descriptor sets for each pipeline
  - Update descriptor sets with buffers/textures
  - Consider descriptor set layouts for each shader

#### 5.3 Swapchain Management
- **Action**: Handle window resizing and swapchain recreation
- **Files**: `src/VulkanSwapchain.hpp/cpp`
- **Details**:
  - Detect window resize events
  - Recreate swapchain with new dimensions
  - Recreate framebuffers and render passes if needed
  - Handle swapchain image count changes

### Phase 6: Optimization and Cleanup

#### 6.1 Remove OpenGL Dependencies
- **Action**: Clean up OpenGL code
- **Files**: All source files
- **Details**:
  - Remove GLEW includes and initialization
  - Remove OpenGL-specific code paths
  - Update CMakeLists.txt to remove GLEW
  - Keep GLFW for window management (it supports Vulkan)

#### 6.2 Performance Optimization
- **Action**: Optimize Vulkan rendering
- **Details**:
  - Use command buffer pools for reuse
  - Implement double/triple buffering for uniform buffers
  - Use push constants for small, frequently changed data
  - Consider using compute shaders for particle updates (future enhancement)
  - Implement proper pipeline caching

#### 6.3 Error Handling and Validation
- **Action**: Add comprehensive error checking
- **Details**:
  - Enable validation layers in debug builds
  - Check all Vulkan API return values
  - Implement proper error messages
  - Add debug markers for RenderDoc/NSight

## Implementation Order

### Step 1: Basic Vulkan Setup (Week 1)
1. Set up Vulkan SDK and dependencies
2. Create VulkanContext class
3. Initialize Vulkan instance and device
4. Create swapchain and surface
5. Verify basic initialization works

### Step 2: Shader and Pipeline Setup (Week 1-2)
1. Convert shaders to SPIR-V
2. Create VulkanShader class
3. Create basic graphics pipeline
4. Set up descriptor set layouts
5. Test with simple triangle rendering

### Step 3: Buffer and Memory Management (Week 2)
1. Create VulkanBuffer class
2. Implement memory allocation
3. Migrate SSBO to storage buffer
4. Test buffer updates and GPU access

### Step 4: Framebuffer and Render Pass (Week 2-3)
1. Create render pass for offscreen rendering
2. Create framebuffer class
3. Migrate render-to-texture functionality
4. Test offscreen rendering

### Step 5: Main Rendering Loop (Week 3)
1. Implement command buffer recording
2. Migrate base shader rendering
3. Migrate screen compositing
4. Test full rendering pipeline

### Step 6: UI Rendering (Week 3-4)
1. Migrate circle drawing
2. Migrate rectangle drawing
3. Migrate text rendering
4. Test UI overlay

### Step 7: Rigid Body Rendering (Week 4)
1. Migrate rigid body shader
2. Implement vertex buffer for polygons
3. Test rigid body visualization

### Step 8: Cleanup and Testing (Week 4-5)
1. Remove OpenGL code
2. Fix any remaining issues
3. Performance testing and optimization
4. Documentation

## Dependencies to Add

### Required
- **Vulkan SDK**: Latest version (1.3+ recommended)
- **GLFW**: Already present, supports Vulkan surfaces
- **GLM**: Already present, works with Vulkan
- **SPIR-V Compiler**: `glslc` (from Vulkan SDK) or `glslangValidator`

### Optional but Recommended
- **Vulkan Memory Allocator (VMA)**: Easier memory management
- **Vulkan-Hpp**: C++ bindings for Vulkan (more type-safe)

## Key Differences: OpenGL vs Vulkan

### Immediate Mode → Command Buffers
- **OpenGL**: `glBegin()`, `glVertex2f()`, `glEnd()`
- **Vulkan**: Record commands in command buffer, submit to queue

### State Management
- **OpenGL**: Implicit state machine
- **Vulkan**: Explicit pipeline state objects

### Memory Management
- **OpenGL**: Driver manages memory
- **Vulkan**: Explicit memory allocation and management

### Synchronization
- **OpenGL**: Implicit synchronization
- **Vulkan**: Explicit semaphores, fences, barriers

### Shader Uniforms
- **OpenGL**: `glUniform*()` calls
- **Vulkan**: Descriptor sets and push constants

### Framebuffers
- **OpenGL**: `glBindFramebuffer()`, `glFramebufferTexture()`
- **Vulkan**: Render passes, framebuffer objects

## Potential Challenges and Solutions

### Challenge 1: Immediate Mode Removal
- **Problem**: Heavy use of immediate mode rendering
- **Solution**: Pre-generate vertex buffers for common shapes, use instancing

### Challenge 2: Shader Uniform Access
- **Problem**: OpenGL uses `glUniform*()` which is straightforward
- **Solution**: Use descriptor sets for textures/buffers, push constants for small data

### Challenge 3: Synchronization Complexity
- **Problem**: Vulkan requires explicit synchronization
- **Solution**: Create helper functions/classes to manage semaphores and fences

### Challenge 4: Memory Management
- **Problem**: Vulkan memory management is complex
- **Solution**: Use VMA library or create wrapper classes

### Challenge 5: Debugging
- **Problem**: Vulkan errors are less obvious than OpenGL
- **Solution**: Enable validation layers, use RenderDoc for debugging

## Testing Strategy

1. **Unit Tests**: Test each Vulkan wrapper class individually
2. **Integration Tests**: Test rendering pipeline components
3. **Visual Tests**: Compare OpenGL and Vulkan output side-by-side
4. **Performance Tests**: Benchmark frame times and memory usage
5. **Compatibility Tests**: Test on different GPUs/drivers

## Success Criteria

- [ ] Application runs with Vulkan renderer
- [ ] Particle simulation renders correctly
- [ ] UI elements display correctly
- [ ] Rigid bodies render correctly (if enabled)
- [ ] Performance is comparable or better than OpenGL
- [ ] No memory leaks or validation errors
- [ ] Window resizing works correctly
- [ ] All shader effects work as before

## Notes

- GLFW already supports Vulkan, so window management can stay the same
- GLM works with Vulkan (just math library)
- Box2D and FreeType are independent of rendering API
- Consider keeping OpenGL code in a separate branch for reference
- May want to add runtime API selection (OpenGL/Vulkan) in the future

## Estimated Timeline

- **Total Duration**: 4-5 weeks for complete migration
- **Minimum Viable Product**: 2-3 weeks (basic rendering working)
- **Full Feature Parity**: 4-5 weeks (including UI and optimizations)

## Resources

- Vulkan Tutorial: https://vulkan-tutorial.com/
- Vulkan Specification: https://www.khronos.org/registry/vulkan/specs/
- VMA Documentation: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
- Vulkan Samples: https://github.com/KhronosGroup/Vulkan-Samples

