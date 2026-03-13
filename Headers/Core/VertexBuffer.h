#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include"QueueFamilyIndicesHeader.h"


class DeviceManager;

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

bool operator==(const Vertex& other) const {
	return pos == other.pos && color == other.color && texCoord == other.texCoord;
}



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



	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

public:

	BufferManager(DeviceManager * deviceManager,VkCommandPool CommandPool);
	void LoadModel();
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

	const std::vector<uint32_t> GetIndices()
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

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}


};