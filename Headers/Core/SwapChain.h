#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include"SwapChainSupportDetails.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class DeviceManager;

//The swap chain is essentially a queue of images that are
// waiting to be presented to the screen.Our application will acquire such an image to draw to it,
//and then return it to the queue
//The general purpose of the swap chain is to synchronize the presentation of images 
//	with the refresh rate of the screen.

class SwapChain
{
public:
	SwapChain(DeviceManager * deviceManager);
	static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	void createSwapChain(VkPhysicalDevice physiscaldevice, VkDevice logicalDevice, VkSurfaceKHR surface,GLFWwindow * window);
	void createImageViews(VkDevice logicalDevice);
	void DestroySwapChain(VkDevice logicalDevice);
	void DestroyImageViews(VkDevice logicalDevice);
	void Init();
	 VkFormat GetSwapChainImageFormat();
	 std::vector<VkImageView> GetSwapChainImageViews();
	 VkExtent2D GetExtend();
	 VkSwapchainKHR Get();
private:
	VkSwapchainKHR swapChain;
	DeviceManager * m_DeviceManager;

	//we store the format and extend we have chosen for this swap chain 
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	


    //retrieve the handles of the image 
	std::vector<VkImage> swapChainImages;

	//need an interfacet to the images 
	std::vector<VkImageView> swapChainImageViews;



	//find the right settings for the best possible swap chain.
    //There are three types of settings to determine :
    //Surface format(color depth)
    //Presentation mode(conditions for "swapping" images to the screen)
    //Swap extent(resolution of images in swap chain)

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities,GLFWwindow* window);




};