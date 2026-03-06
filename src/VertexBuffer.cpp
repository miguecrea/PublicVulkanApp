#include"../Headers/Core/VertexBuffer.h"
#include <stdexcept>
#include <iostream>
#include"../Headers/Core/DeviceManager.h"

BufferManager::BufferManager(DeviceManager * deviceManager) :
	m_deviceManager{deviceManager}
{
}

VkBuffer BufferManager::GetVertexBuffer()
{
	return vertexBuffer;
}

void BufferManager::CreateVertexBuffer(VkCommandPool commandPool)
{


	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();


	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
	//VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.
	// 

	void* data;
	vkMapMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory);

	////staging buffer 
	// The vertexBuffer is now allocated from a memory type that is device local,
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer, vertexBufferMemory);

     // he vertexBuffer is now allocated from a memory type that is device local, which generally means that we're not able to use vkMapMemory. 
     // However, we can copy data from the stagingBuffer to the vertexBuffer.
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize,commandPool);
	vkDestroyBuffer(m_deviceManager->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_deviceManager->GetLogicalDevice(), stagingBufferMemory, nullptr);




#pragma region VertexBufferExplanation


	//VkBufferCreateInfo bufferInfo{};
	//bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	//bufferInfo.size = sizeof(vertices[0]) * vertices.size();
	//bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	//// Just like the images in the swap chain, buffers can also be
	//// owned by a specific queue family or be shared between 
	//// multiple at the same time. The buffer will only be used 
	//// from the graphics queue, 
	//// so we can stick to exclusive access.
	//bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	//if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
	//	throw std::runtime_error("failed to create vertex buffer!");
	//}


	//// he first step of allocating memory for the buffer is to query its memory requirements u
	//VkMemoryRequirements memRequirements;
	//vkGetBufferMemoryRequirements(m_device, vertexBuffer, &memRequirements);

	//VkMemoryAllocateInfo allocInfo{};
	//allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	//allocInfo.allocationSize = memRequirements.size;

	//allocInfo.memoryTypeIndex = DeviceManager::findMemoryType(memRequirements.memoryTypeBits,
	//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,m_physicalDevice);


	//if (vkAllocateMemory(m_device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
	//	throw std::runtime_error("failed to allocate vertex buffer memory!");
	//}
	////If memory allocation was successful aasociate this memory with the buffer ,


	//vkBindBufferMemory(m_device, vertexBuffer, vertexBufferMemory, 0);



	//// here we just copy vertex data into the vertex 

	//// hich ensures that the mapped memory always matches
	//// the contents of the allocated memory.
	//void * data;
	//vkMapMemory(m_device,vertexBufferMemory, 0, bufferInfo.size, 0, &data);
	//memcpy(data, vertices.data(), (size_t)bufferInfo.size);
	//vkUnmapMemory(m_device, vertexBufferMemory);

#pragma endregion

}

void BufferManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer  & buffer, VkDeviceMemory & bufferMemory)
{

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_deviceManager->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_deviceManager->GetLogicalDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = DeviceManager::findMemoryType(memRequirements.memoryTypeBits, properties, m_deviceManager->GetPhysicalDevice());

	if (vkAllocateMemory(m_deviceManager->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(m_deviceManager->GetLogicalDevice(), buffer, bufferMemory, 0);
}


void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool)
{

	// Memory transfer operations are executed using command buffers
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_deviceManager->GetLogicalDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;// // We're only going to use the command buffer once and wait with returning from the function until the copy operation has finished executing. 


	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	VkBufferCopy copyRegion{};
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	//end command 
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_deviceManager->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	// wait for the transfer queue to become idle //we could use fences but to have multiple transfers simulaniosly 

	vkQueueWaitIdle(m_deviceManager->GetGraphicsQueue());

	vkFreeCommandBuffers(m_deviceManager->GetLogicalDevice(), commandPool, 1, &commandBuffer);
}


void BufferManager::DestroyBuffer()
{
	vkDestroyBuffer(m_deviceManager->GetLogicalDevice(), vertexBuffer, nullptr);
}

void BufferManager::FreeMemoryBuffer()
{
	vkFreeMemory(m_deviceManager->GetLogicalDevice(), vertexBufferMemory, nullptr);
}


