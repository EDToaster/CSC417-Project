/**
 * glError.hpp
 * Contributors:
 *      * Arthur Sonzogni (author)
 * Licence:
 *      * MIT
 * 
 * NOTE: This file is kept for compatibility but stubbed out during Vulkan migration.
 * OpenGL error checking is not applicable to Vulkan.
 * TODO: Phase 2 - Implement Vulkan error checking using validation layers.
 */

#ifndef OPENGL_CMAKE_SKELETON_GLERROR_HPP
#define OPENGL_CMAKE_SKELETON_GLERROR_HPP

// Stubbed OpenGL error checking function
// In Vulkan, error checking is done via validation layers
// usage :
//      glCheckError(__FILE__,__LINE__);  // Does nothing in Vulkan build
void glCheckError(const char* file, unsigned int line);

#endif  // OPENGL_CMAKE_SKELETON_GLERROR_HPP
