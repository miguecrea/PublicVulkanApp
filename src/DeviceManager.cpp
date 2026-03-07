#include"../Headers/Core/DeviceManager.h"
#include"../Headers/Core/InstanceManager.h"
#include <vector>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <set>
#include"../Headers/Core/SwapChain.h"

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
        if (isDeviceSuitable(device))
        {
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
    m_indices = findQueueFamilies(m_physicalDevice);

    //set in case they are In the same Index 
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { m_indices.graphicsFamily.value(), m_indices.presentFamily.value() };

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
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    //enable swap chain extension
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();


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
    vkGetDeviceQueue(m_LogicalDevice,m_indices.graphicsFamily.value(), 0, &graphicsQueue);
    //store present family in a handle 
    vkGetDeviceQueue(m_LogicalDevice, m_indices.presentFamily.value(), 0, &presentQueue);




}

void DeviceManager::DestroyLogicalDevice()
{
    vkDestroyDevice(m_LogicalDevice, nullptr);
}

const VkPhysicalDevice & DeviceManager::GetPhysicalDevice()
{
    return m_physicalDevice;
}

VkDevice DeviceManager::GetLogicalDevice()
{
    return m_LogicalDevice;
}

QueueFamilyIndices DeviceManager::GetFamilyIndices()
{
    return m_indices;
}

VkQueue DeviceManager::GetGraphicsQueue()
{
    return graphicsQueue;
}

VkQueue DeviceManager::GetPresentQueue()
{
    return presentQueue;
}



// // Graphics cards can offer different types of memory to allocate from. Each type of memory varies in terms of allowed operations
// and performance characteristics
// We may have more than one desirable property, so we should check if the result 
// of the bitwise AND is not just non-zero, but equal to the desired properties bit field. If there is a memory type suitable for the buffer that also has
// all of the properties we need,

uint32_t DeviceManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties,VkPhysicalDevice Physicaldevice)
{
 
    // First we need to query info about the available types of memory using 
  
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(Physicaldevice,&memProperties);


    //find if a memory is suitable based on the bit filter we passed on top
    //and it is properties like being able to map memory like  (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
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

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(device,m_Surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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

bool DeviceManager::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto & extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}