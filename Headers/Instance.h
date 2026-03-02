#pragma once
#include <vulkan/vulkan.h>
#include<vector>

class Instance
{
private:
    VkInstance m_instance{VK_NULL_HANDLE};
public:
    Instance();
   void CreateInstance();
   VkInstance GetInstance() const;
   VkDebugUtilsMessengerEXT GetdebugMessenger() const;
   void setupDebugMessenger(); 
   bool m_EnableValidationLayers{ true };
   
private:


   //Debug

   VkDebugUtilsMessengerEXT debugMessenger;
   std::vector<const char*> getRequiredExtensions();
   bool checkValidationLayerSupport();
   void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
   void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & createInfo);
   VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
   void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks * pAllocator);
    
};