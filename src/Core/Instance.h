#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Instance
{
public:
    Instance() = default;
    ~Instance() = default;

    void Create();
    void Destroy();

    void SetupDebugMessenger();
    void DestroyDebugMessenger();

    VkInstance Get() const { return m_Instance; }
    bool ValidationEnabled() const { return m_EnableValidation; }

    const std::vector<const char*> ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

private:
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    bool m_EnableValidation = true;

    bool CheckValidationSupport();
    std::vector<const char*> GetRequiredExtensions();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};
