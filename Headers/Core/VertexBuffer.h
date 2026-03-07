#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include"QueueFamilyIndicesHeader.h"


class DeviceManager;

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;
};
class BufferManager
{
private:

	VkCommandPool m_CommandPool;
	DeviceManager * m_deviceManager;
	VkCommandBuffer m_commandBuffer;
	//vertex buffer 
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	//index buffer 
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;


	const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

public:

	BufferManager(DeviceManager * deviceManager,VkCommandPool CommandPool);
	VkBuffer GetVertexBuffer();
	VkBuffer GetIndexBuffer();
	void CreateVertexBuffer();
	void CreateIndexBuffer();




	//void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool);


	void DestroyIndexBuffer();
	void FreeIndexMemoryBuffer();

	void DestroyVertexBuffer();
	void FreeVertexMemoryBuffer();

	const std::vector<Vertex>& GetVertices()
	{
		return vertices;
	}

	const std::vector<uint16_t> GetIndices()
	{
		return indices;
	}
	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}



};