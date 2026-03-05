
#include"../Headers/Core/SwapChain.h"
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <stdexcept>
#include <iostream>
#include"../Headers/Core/DeviceManager.h"
SwapChain::SwapChain(DeviceManager * deviceManager):
	m_DeviceManager{deviceManager}
{
}

void SwapChain::createSwapChain(VkPhysicalDevice physiscaldevice, VkDevice logicalDevice,VkSurfaceKHR surface,GLFWwindow * window)
{
	SwapChainSupportDetails swapChainSupport = SwapChain::querySwapChainSupport(physiscaldevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

	//get + 1 image of the minimum 
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//specifies what kind of operations we'll use the images in the swap chain for.
	// 
	// In this tutorial we're going to render directly to them, which means that they're 
	// used as color attachment. It is also possible that you'll render images to a separate
	// image first to perform operations like post - processing.In that case you may use a value
	// like VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the 
	// rendered image to a swap chain image.

	//Next, we need to specify how to handle swap chain images that will be used across 
	// multiple queue families.
	//if the graphics queue family is different from the presentation queue.We'll be drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue. There are two ways to handle images that are accessed from multiple queues:

	QueueFamilyIndices indices = m_DeviceManager->findQueueFamilies(physiscaldevice);

	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

   // VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family.This option offers the best performance.
   // VK_SHARING_MODE_CONCURRENT : Images can be used across multiple queue families without explicit ownership transfers.
	if (indices.graphicsFamily != indices.presentFamily) 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	//if we want a transform on tye swap chain image 
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	// The compositeAlpha field specifies if the alpha channel should be used for blending with other windows in the
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	// don't care about the color of pixels that are obscured, for example because another window is in front of them.
	createInfo.clipped = VK_TRUE;

	//when window is resized so we need to recreate swap chain 
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);

	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}


VkFormat SwapChain::GetSwapChainImageFormat()
{
	return swapChainImageFormat;
}
std::vector<VkImageView> SwapChain::GetSwapChainImageViews()
{
	return swapChainImageViews;
}
VkExtent2D SwapChain::GetExtend()
{
	return swapChainExtent;
}
void SwapChain::createImageViews(VkDevice logicalDevice)
{
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		// spcify how image data should be interpreted 
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  //1d 2d 3d texture etc 
		createInfo.format = swapChainImageFormat;

		// The components field allows you to swizzle the color channels around
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. Our images will be used as color targets without any mipmapping 
		// levels or multiple layers.

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}

	}
}

void SwapChain::DestroySwapChain(VkDevice logicalDevice)
{
	vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
}

void SwapChain::DestroyImageViews(VkDevice logicalDevice)
{
	for (auto imageView : swapChainImageViews) 
	{
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}
}


SwapChainSupportDetails SwapChain::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{

	//check if swap chain support present Modes,formats etc 
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;


}



VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{

	for (const auto & availableFormat : availableFormats)
	{
		// how we will store it SRGB 
		// color space indicates if this color format is supported 
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}


//PRESENTATION FORMAT:

//VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, which may result in tearing.
//VK_PRESENT_MODE_FIFO_KHR : The swap chain is a queue where the display takes an image from the front of the queue when the display is refreshed and the program inserts rendered images at the back of the queue.If the queue is full then the program has to wait.This is most similar to vertical sync as found in modern games.The moment that the display is refreshed is known as "vertical blank".
//VK_PRESENT_MODE_FIFO_RELAXED_KHR : This mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank.Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives.This may result in visible tearing.
//VK_PRESENT_MODE_MAILBOX_KHR : This is another variation of the second mode.Instead of blocking the application when the queue is full, the images that are already queued are simply replaced with the newer ones.This mode can be used to render frames as fast as possible while still avoiding tearing, resulting in fewer latency issues than standard vertical sync.This is commonly known as "triple buffering", although the existence of three buffers alone does not necessarily mean that the framerate is unlocked.

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

//The swap extent is the resolution of the swap chain images and it's
// almost always exactly equal to the resolution of the window that we're drawing to in pixels 

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;

		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

