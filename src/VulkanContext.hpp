#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <optional>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

namespace Vulkan {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    class VulkanContext {
    public:
        VulkanContext();
        ~VulkanContext();

        // Initialize Vulkan instance and device
        void Initialize(VkSurfaceKHR surface);
        void Cleanup();

        // Getters
        VkInstance GetInstance() const { return instance; }
        VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        VkDevice GetDevice() const { return device; }
        VkQueue GetGraphicsQueue() const { return graphicsQueue; }
        VkQueue GetPresentQueue() const { return presentQueue; }
        QueueFamilyIndices GetQueueFamilyIndices() const { return queueFamilyIndices; }

        // Check if validation layers are enabled
        bool IsValidationEnabled() const { return enableValidationLayers; }

    private:
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

        QueueFamilyIndices queueFamilyIndices;

        // Validation layer names
        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        // Required device extensions
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        // Helper functions
        void CreateInstance();
        void SetupDebugMessenger();
        void PickPhysicalDevice(VkSurfaceKHR surface);
        void CreateLogicalDevice(VkSurfaceKHR surface);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        bool CheckValidationLayerSupport();
        bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
        std::vector<const char*> GetRequiredExtensions();
    };

} // namespace Vulkan

