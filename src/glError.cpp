/**
 * glError.cpp
 * Contributors:
 *      * Arthur Sonzogni (author)
 * Licence:
 *      * MIT
 * 
 * NOTE: This file is kept for compatibility but stubbed out during Vulkan migration.
 * OpenGL error checking is not applicable to Vulkan.
 * TODO: Phase 2 - Implement Vulkan error checking using validation layers.
 */

#include "glError.hpp"

#include <iostream>
#include <string>

using namespace std;

void glCheckError(const char* file, unsigned int line) {
  // Stubbed out - OpenGL error checking not applicable to Vulkan
  // Vulkan uses validation layers for error checking instead
  // This function is kept for compatibility but does nothing
  (void)file;  // Suppress unused parameter warning
  (void)line;  // Suppress unused parameter warning
}
