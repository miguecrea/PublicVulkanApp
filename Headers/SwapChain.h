#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include"SwapChainSupportDetails.h"

class SwapChain
{
public:

    SwapChain() = default;
    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,VkSurfaceKHR surface);
   
private:

};