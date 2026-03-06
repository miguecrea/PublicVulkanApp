#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include<vector>
#include"QueueFamilyIndicesHeader.h"

class InstanceManager;
class DeviceManager
{
public:
    DeviceManager();
    void pickPhysicalDevice(VkInstance Instance);
    void createLogicalDevice(InstanceManager * IntanceManager);
    void DestroyLogicalDevice();
    void SetSurface(VkSurfaceKHR surface) { m_Surface = surface;}
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    const VkPhysicalDevice & GetPhysicalDevice();
    VkDevice GetLogicalDevice();

    QueueFamilyIndices GetFamilyIndices();

    VkQueue GetGraphicsQueue();
    VkQueue GetPresentQueue();

    static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice device);

private:
    QueueFamilyIndices m_indices;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice;
    bool isDeviceSuitable(VkPhysicalDevice device);


    // ACTUAL QUEUE FAMILIES 

    // Graphics queue we query it the the parameter and stored it's index
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    // Needs the surface to find queue families 
    VkSurfaceKHR m_Surface = nullptr;

    const std::vector<const char*> deviceExtensions = 
    {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

   bool checkDeviceExtensionSupport(VkPhysicalDevice device);

};