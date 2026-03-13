#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Instance;

class Device
{
public:
    Device() = default;
    ~Device() = default;

    void Init(Instance* instance, VkSurfaceKHR surface);
    void Destroy();

    VkDevice GetLogical() const { return m_Device; }
    VkPhysicalDevice GetPhysical() const { return m_PhysicalDevice; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    VkQueue GetPresentQueue() const { return m_PresentQueue; }
    const QueueFamilyIndices& GetQueueFamilies() const { return m_Indices; }

    static uint32_t FindMemoryType(VkPhysicalDevice physical, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

private:
    VkDevice m_Device = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    QueueFamilyIndices m_Indices;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

    const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    void PickPhysicalDevice(VkInstance instance);
    void CreateLogicalDevice(Instance* instance);
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckExtensionSupport(VkPhysicalDevice device);
};
