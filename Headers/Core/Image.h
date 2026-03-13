#pragma once

#include <vulkan/vulkan.h>
#include<vector>
class DeviceManager;
class Image
{

public:

	Image(DeviceManager * deviceManager);


	void createTextureImage();
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
		VkDeviceMemory& imageMemory);
	void DestroyImage();

	void createTextureImageView();
	void destroyTextureImageView();
	void createTextureSampler();
	void DestroyTextureSampler();


	void DestroyDepthBufferingRelated();

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkImageView createImageView(VkDevice device,VkImage image, VkFormat format);
	VkImageView createImageViewWithAspectFlags(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	
	VkFormat findSupportedFormat(const std::vector<VkFormat> & candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();

	bool hasStencilComponent(VkFormat format) 
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;


	}

	void SetCommandPool(VkCommandPool command);

	VkImageView GetDepthImageView();

	void CreateDepthResources(VkExtent2D swapChainExtend);

	VkImageView GetTextureImageView();
	VkSampler GetTextureSampler();
private:

	VkSampler textureSampler;
	//image view for the texture 
	VkImageView textureImageView;


	//for depth buffering 
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;



	DeviceManager * m_deviceManager;
	VkCommandPool m_CommandPool;

	//image staging buffer 
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
};