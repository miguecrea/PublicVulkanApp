#include "../Headers/Core/CommandManager.h"
#include "../Headers/Core/VertexBuffer.h"
#include "../Headers/Core/DescriptorSetLayout.h"
#include <stdexcept>
#include <iostream>

CommandManager::CommandManager(VkDevice device, const QueueFamilyIndices & queueFamilyIndices):
	m_Device{device},
	m_QueueFamilyIndices{queueFamilyIndices}
{
}

void CommandManager::CreateCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

   //VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are rerecorded with new commands very often(may change memory allocation behavior)
   //VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : Allow command buffers to be rerecorded individually, without this flag they all have to be reset together

	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = m_QueueFamilyIndices.graphicsFamily.value();

	// Command buffers are executed by submitting them on one of the device queues,
	// like the graphics and presentation


	//we want draw commands so we use the index of this queuue
	if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void CommandManager::DestroyCommandPool()
{

	vkDestroyCommandPool(m_Device,m_CommandPool,nullptr);

}

void CommandManager::createCommandBuffer()
{
	// VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, 
	// but cannot be called from other command buffers.
	

    // VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, 
	// but can be called from primary command buffers.


	//we need multiple command buffers to use frmaes in flight so GPu does not stall when 
	//waiting fdor the next frame 
	
	m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	if (vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

const VkCommandBuffer & CommandManager::GetComandBuffer()
{
	return commandBuffer;
}

void CommandManager::recordCommandBuffer(VkCommandBuffer commandBuffer,
	uint32_t imageIndex,VkRenderPass renderPass,const std::vector<VkFramebuffer> & frameBuffer,
	VkPipeline graphicsPipeline,VkExtent2D swapChainExtent,BufferManager * vertexbuffer,
	DescriptorSetLayout * descriptorSet,VkPipelineLayout pipleinelayout)
{
	//always start by begin command buffer 
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

     //VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
      //VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : This is a secondary command buffer that will be entirely within a single render pass.
      //VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : The command buffer can be resubmitted while it is also already pending execution.
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	// start a render pass 
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	//we created a frame buffer for each image index
	renderPassInfo.framebuffer = frameBuffer[imageIndex];

	//extend
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainExtent;
	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };  //clear value for 
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	//VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
     //VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be executed from secondary command buffers
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	//since in pipeline they are dynamic 
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);



	VkBuffer vertexBuffers[] = { vertexbuffer->GetVertexBuffer()};
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, vertexbuffer->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

//vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
//instanceCount : Used for instanced rendering, use 1 if you're not doing that.
//firstVertex : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
//firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.


	//bind descriptorSets

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,pipleinelayout, 0, 1, &descriptorSet->GetdescriptorSets()[imageIndex], 0, nullptr);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vertexbuffer->GetIndices().size()), 1, 0, 0, 0);


	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

const std::vector<VkCommandBuffer>& CommandManager::GetCommandBuffersVector() const
{

	return m_CommandBuffers;
}

VkCommandPool CommandManager::GetCommandPool()
{
	return m_CommandPool;
}
