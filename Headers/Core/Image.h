#pragma once

#include <vulkan/vulkan.h>

class DeviceManager;
class Image
{

public:

	Image(DeviceManager * deviceManager, VkCommandPool commandPool);


	void createTextureImage();
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
		VkDeviceMemory& imageMemory);
	void DestroyImage();

	void createTextureImageView();
	void destroyTextureImageView();
	void createTextureSampler();
	void DestroyTextureSampler();

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkImageView createImageView(VkDevice device,VkImage image, VkFormat format);
	
private:

	VkSampler textureSampler;
	//image view for the texture 
	VkImageView textureImageView;

	DeviceManager * m_deviceManager;
	VkCommandPool m_CommandPool;

	//image staging buffer 
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
};