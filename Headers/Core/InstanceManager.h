#pragma once
#include <vulkan/vulkan.h>
#include<vector>

class InstanceManager
{
private:
    VkInstance m_instance{VK_NULL_HANDLE};
public:
    InstanceManager();
   void CreateInstance();
   VkInstance GetVulkanInstance() const;
   void DestroyValidationLayers();
   void setupDebugMessenger(); 
   bool AreValidationLayersEnabled() { return m_EnableValidationLayers;}
   const std::vector<const char*> validationLayers =
   {
       "VK_LAYER_KHRONOS_validation"
   };

private:
   //Debug

   bool m_EnableValidationLayers{ true };
   VkDebugUtilsMessengerEXT GetdebugMessenger() const;
   VkDebugUtilsMessengerEXT m_debugMessenger;
   std::vector<const char*> getRequiredExtensions();
   bool checkValidationLayerSupport();
   void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
   VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
   void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks * pAllocator);
    
   static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void * pUserData);

};