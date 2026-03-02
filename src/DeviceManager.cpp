#include"../Headers/DeviceManager.h"
#include"../Headers/InstanceManager.h"
#include <vector>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <set>

DeviceManager::DeviceManager()
{
}

void DeviceManager::pickPhysicalDevice(VkInstance Instance)
{
 
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());

    for (const auto & device : devices)
    {
        //if we found a device that support graphics queue family 
        //we stored the Index
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}



//Vulkan lets you assign priorities to queues to influence the scheduling of 
// command buffer execution using floating point numbers between 
//0.0 and 1.0.This is required even if there is only a single queue :

void DeviceManager::createLogicalDevice(InstanceManager * IntanceManager)
{

    //QUEUE FAMILY
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    //set in case they are In the same Index 
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) 
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;  //index 
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }


    // CREATE LOGICAL DEVICE USING THE DATA OF THE QUUE Create Info 
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;


    // Specify device specific  extensions and validation layers for logical device 
    //for now we dont need any device specifoc functionality 
    createInfo.enabledExtensionCount = 0;
    if (IntanceManager->AreValidationLayersEnabled())
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(IntanceManager->validationLayers.size());
        createInfo.ppEnabledLayerNames = IntanceManager->validationLayers.data();
    }
    else 
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    //we stored the graphics queue family in a Handle 
    vkGetDeviceQueue(m_LogicalDevice,indices.graphicsFamily.value(), 0, &graphicsQueue);
    //store present family in a handle 
    vkGetDeviceQueue(m_LogicalDevice, indices.presentFamily.value(), 0, &presentQueue);




}

void DeviceManager::DestroyLogicalDevice()
{
    vkDestroyDevice(m_LogicalDevice, nullptr);
}

//EXPLANATION :
#pragma region QueueFamilyExplanation
 
//Queue families
//It has been briefly touched upon before that almost every operation
//in Vulkan, anything from drawing to uploading textures, 
//requires commands to be submitted to a queue.

//For example, there could be a queue family that only allows 
// processing of compute commands or one that only allows memory 
// transfer related commands.
// 
//We need to check which queue families are supported by the 
// device and which one of these supports
//the commands that we want to use.

#pragma endregion

bool DeviceManager::isDeviceSuitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.isComplete();
}

QueueFamilyIndices DeviceManager::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto & queueFamily : queueFamilies)
    {
        //seee if the graphics and present are supported 
        //check all of families could be they are on differen or the same 
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;

        if (m_Surface == nullptr)
        {
            std::cout << "Surface Is Null\n";
        }

        vkGetPhysicalDeviceSurfaceSupportKHR(device,i,m_Surface,&presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }



        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}
