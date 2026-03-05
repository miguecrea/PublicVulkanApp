#include"../Headers/Core/RenderPass.h"
#include <stdexcept>
#include <iostream>
//
//In Vulkan, a render pass is an object that defines :
//
//What framebuffer attachments you will use(color, depth, resolve)
//
//How those attachments are treated(load / store operations)
//
//The layout transitions of those images
//
//The execution order via subpasses and dependencies
//
//
//

RenderPass::RenderPass(VkDevice device, VkFormat swapChainImageFormat) :
	m_Device{ device },
	m_ImageFormat{ swapChainImageFormat }
{
}

// “For this section of rendering, I will render into these images, with these rules.”

// A single render pass can consist of multiple subpasses.
void RenderPass::CreateRenderPass()
{

	//called before creating graphics pipeline 
	// 
   // Before we can finish creating the pipeline, we need to tell Vulkan about the framebuffer
   // attachments that will be used while rendering.We need to specify how many color and depth buffers there will be,

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_ImageFormat; //should match format of the swap chain images 
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	//determine what to do with the data in the attachment before rendering and after rendering.

	//VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
	//VK_ATTACHMENT_LOAD_OP_CLEAR : Clear the values to a constant at the start
	//VK_ATTACHMENT_LOAD_OP_DONT_CARE : Existing contents are undefined; we don't care about them
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;


	//VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
	//VK_ATTACHMENT_STORE_OP_DONT_CARE : Contents of the framebuffer will be undefined after the rendering operation

	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;


	// Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format,
	// however the layout of the pixels in memory can change based on what you're trying to do with an image.

	 //Some of the most common layouts are :

	/// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
	// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : Images to be presented in the swap chain
	// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Images to be used as destination for a memory copy operation

	 //images need to be traistioned to a specific layout

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//we want the iage to be ready for presentation 
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Every subpass references one or more of the attachments
	//  to reference by its index in the attachment descriptions array. Our array consists of a single VkAttachmentDescription, so its index is 0. The layout specifies which layout we would like the attachment to have during a subpass that uses this reference. Vulkan will automatically transition the attachment to this layout when 
	// the subpass is started. 
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;



	// subpasses have attacments and can have many attachments 
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	// The index of the attachment in this array is directly referenced from the fragment 
	// shader with the layout(location = 0) out vec4 outColor directive!


	//The following other types of attachments can be referenced by a subpass:

//pInputAttachments: Attachments that are read from a shader
//pResolveAttachments : Attachments used for multisampling color attachments
//pDepthStencilAttachment : Attachment for depth and stencil data
//pPreserveAttachments : Attachments that are not used by this subpass, but for which the data must be preserved
//




	// we say that we need to wait till color attachment 
	//so we add a dependecy here 


	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	//The next two fields specify the operations to wait on and the stages
	// in which these operations occur.We need to wait for the swap chain to finish reading
	// from the image before we can access it.This can be accomplished by waiting on the color attachment output stage itself.

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;





	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;


	//dependency is added here 
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

}

void RenderPass::DestroyRenderPass()
{
	vkDestroyRenderPass(m_Device,m_RenderPass,nullptr);
}
