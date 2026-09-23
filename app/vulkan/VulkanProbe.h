#pragma once

#include <string>

namespace ufx {

struct VulkanCaps {
    bool loaderPresent = false;      // vulkan-1.dll present on the system
    bool anyPhysicalDevice = false;  // At least one Vulkan physical device
    std::string apiVersion;          // Highest instance API version, if resolved
};

// Uses LoadLibrary + GetProcAddress and does not link vulkan-1.lib, so builds
// remain vendor-agnostic and never fail because the Vulkan SDK is absent.
VulkanCaps ProbeVulkan();

}
