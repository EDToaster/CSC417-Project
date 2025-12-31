#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <iostream>
#include "VulkanContext.hpp"

namespace Vulkan {

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanSwapchain {
    public:
        VulkanSwapchain(VulkanContext* context, VkSurfaceKHR surface, uint32_t width, uint32_t height);
        ~VulkanSwapchain();

        void Recreate(uint32_t width, uint32_t height);
        void Cleanup();

        // Getters
        VkSwapchainKHR GetSwapchain() const { return swapchain; }
        VkFormat GetImageFormat() const { return imageFormat; }
        VkExtent2D GetExtent() const { return extent; }
        const std::vector<VkImage>& GetImages() const { return swapchainImages; }
        const std::vector<VkImageView>& GetImageViews() const { return swapchainImageViews; }
        uint32_t GetImageCount() const { return static_cast<uint32_t>(swapchainImages.size()); }

    private:
        VulkanContext* context;
        VkSurfaceKHR surface;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat imageFormat;
        VkExtent2D extent;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;

        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);
        void CreateSwapchain(uint32_t width, uint32_t height);
        void CreateImageViews();
    };

} // namespace Vulkan

