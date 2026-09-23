#include "VulkanProbe.h"

#ifdef _WIN32
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace ufx {

namespace {
using PFN_vkEnumerateInstanceVersion = int(__stdcall*)(uint32_t*);
using PFN_vkCreateInstance = int(__stdcall*)(const void*, const void*, void**);
using PFN_vkDestroyInstance = void(__stdcall*)(void*, const void*);
using PFN_vkEnumeratePhysicalDevices = int(__stdcall*)(void*, uint32_t*, void**);

struct VkApplicationInfo {
    int      sType = 0;
    const void* pNext = nullptr;
    const char* pApplicationName = "UniversalFrameFX";
    uint32_t applicationVersion = 0;
    const char* pEngineName = "UFX";
    uint32_t engineVersion = 0;
    uint32_t apiVersion = (1u<<22) | (1u<<12);
};

struct VkInstanceCreateInfo {
    int      sType = 1;
    const void* pNext = nullptr;
    uint32_t flags = 0;
    const VkApplicationInfo* pApplicationInfo = nullptr;
    uint32_t enabledLayerCount = 0;
    const char* const* ppEnabledLayerNames = nullptr;
    uint32_t enabledExtensionCount = 0;
    const char* const* ppEnabledExtensionNames = nullptr;
};
}

VulkanCaps ProbeVulkan() {
    VulkanCaps caps{};
    HMODULE lib = ::LoadLibraryW(L"vulkan-1.dll");
    if (!lib) return caps;
    caps.loaderPresent = true;

    auto gpa = reinterpret_cast<FARPROC(__stdcall*)(void*, const char*)>(
        ::GetProcAddress(lib, "vkGetInstanceProcAddr"));
    if (!gpa) { ::FreeLibrary(lib); return caps; }

    auto vkEnumVer = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
        gpa(nullptr, "vkEnumerateInstanceVersion"));
    uint32_t apiVersion = (1u<<22);
    if (vkEnumVer) vkEnumVer(&apiVersion);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%u.%u.%u",
                  (apiVersion >> 22) & 0x7F,
                  (apiVersion >> 12) & 0x3FF,
                  apiVersion & 0xFFF);
    caps.apiVersion = buf;

    auto vkCreate = reinterpret_cast<PFN_vkCreateInstance>(gpa(nullptr, "vkCreateInstance"));
    if (vkCreate) {
        VkApplicationInfo app{};
        app.apiVersion = apiVersion;
        VkInstanceCreateInfo ci{};
        ci.pApplicationInfo = &app;
        void* inst = nullptr;
        if (vkCreate(&ci, nullptr, &inst) == 0 && inst) {
            auto vkEnumDev = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
                gpa(inst, "vkEnumeratePhysicalDevices"));
            uint32_t count = 0;
            if (vkEnumDev && vkEnumDev(inst, &count, nullptr) == 0 && count > 0) {
                caps.anyPhysicalDevice = true;
            }
            auto vkDestroy = reinterpret_cast<PFN_vkDestroyInstance>(gpa(inst, "vkDestroyInstance"));
            if (vkDestroy) vkDestroy(inst, nullptr);
        }
    }
    ::FreeLibrary(lib);
    return caps;
}

}
#endif
