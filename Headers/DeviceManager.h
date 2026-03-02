#pragma once
#include <vulkan/vulkan.h>
#include <optional>


class InstanceManager;
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() 
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class DeviceManager
{
public:
    DeviceManager();
    void pickPhysicalDevice(VkInstance Instance);
    void createLogicalDevice(InstanceManager * IntanceManager);
    void DestroyLogicalDevice();
    void SetSurface(VkSurfaceKHR surface) { m_Surface = surface;}
private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice;
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);


    // ACTUAL QUEUE FAMILIES 

    // Graphics queue we query it the the parameter and stored it's index
    VkQueue graphicsQueue;
    VkQueue presentQueue;


    // Needs the surface to find queue fa,ilies 
    VkSurfaceKHR m_Surface = nullptr;

};