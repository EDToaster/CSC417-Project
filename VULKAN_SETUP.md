# Vulkan SDK Setup Guide

## Windows Setup

1. **Download and Install Vulkan SDK**
   - Download from: https://vulkan.lunarg.com/sdk/home#windows
   - Install the SDK (typically to `C:\VulkanSDK\<version>`)
   - The installer should automatically set the `VULKAN_SDK` environment variable

2. **Verify Installation**
   - Open a new PowerShell/Command Prompt
   - Run: `echo $env:VULKAN_SDK` (PowerShell) or `echo %VULKAN_SDK%` (CMD)
   - Should show the path like `C:\VulkanSDK\1.3.xxx.x`

3. **If VULKAN_SDK is not set:**
   - Open System Properties → Environment Variables
   - Add new System/User variable:
     - Name: `VULKAN_SDK`
     - Value: `C:\VulkanSDK\<your-version>` (adjust to your installation path)
   - **Restart your terminal/IDE** after setting the variable

4. **Rebuild CMake**
   - Delete your `build/` directory or CMake cache
   - Reconfigure CMake
   - CMake should now find Vulkan

## Linux Setup

1. **Install via package manager:**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install vulkan-sdk
   
   # Or download from: https://vulkan.lunarg.com/sdk/home#linux
   ```

2. **Set VULKAN_SDK (if needed):**
   ```bash
   export VULKAN_SDK=/path/to/vulkan/sdk
   ```

## Alternative: Manual Path Configuration

If you prefer not to use environment variables, you can manually specify the path in CMakeLists.txt:

```cmake
set(VULKAN_SDK_PATH "C:/VulkanSDK/1.3.xxx.x")  # Adjust path
set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib/vulkan-1.lib")
```

## Verification

After setup, CMake should output:
```
-- Found Vulkan SDK via VULKAN_SDK environment variable: C:/VulkanSDK/...
-- Vulkan Include: C:/VulkanSDK/.../Include
-- Vulkan Library: C:/VulkanSDK/.../Lib/vulkan-1.lib
```

## Troubleshooting

- **"Could NOT find Vulkan"**: Make sure VULKAN_SDK is set and you've restarted your terminal/IDE
- **"Missing VULKAN_LIBRARY"**: Check that the library file exists at the expected path
- **"Missing VULKAN_INCLUDE_DIR"**: Check that `vulkan/vulkan.h` exists in the Include directory

## Note on Git Submodules

Vulkan is typically **not** used as a git submodule because:
- It's a large SDK with platform-specific binaries
- It includes tools (glslc compiler) that need to be in PATH
- It's meant to be installed system-wide
- The SDK is updated frequently

If you need a submodule approach, you could use:
- `Vulkan-Headers` (headers only)
- `Vulkan-Loader` (loader library)

But this requires more setup and is not recommended for most projects.

