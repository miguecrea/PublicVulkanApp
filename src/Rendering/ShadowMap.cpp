#include "ShadowMap.h"
#include "../Core/Device.h"
#include "../Core/CommandManager.h"
#include <stdexcept>
#include <array>

// ---------------------------------------------------------------------------
VkFormat ShadowMap::FindDepthFormat() const
{
    const VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    for (VkFormat fmt : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Device->GetPhysical(), fmt, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return fmt;
    }
    throw std::runtime_error("ShadowMap: no supported depth format found!");
}

// ---------------------------------------------------------------------------
void ShadowMap::TransitionLayout(CommandManager* cmdManager, VkFormat format,
                                  VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer cmd = cmdManager->BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = m_Image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
        0, nullptr, 0, nullptr, 1, &barrier);

    cmdManager->EndSingleTimeCommands(cmd);
}

// ---------------------------------------------------------------------------
void ShadowMap::Create(Device* device, CommandManager* cmdManager)
{
    m_Device = device;
    VkFormat depthFormat = FindDepthFormat();

    // ----- Depth image -----
    VkImageCreateInfo imgInfo{};
    imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType     = VK_IMAGE_TYPE_2D;
    imgInfo.format        = depthFormat;
    imgInfo.extent        = { SIZE, SIZE, 1 };
    imgInfo.mipLevels     = 1;
    imgInfo.arrayLayers   = 1;
    imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(m_Device->GetLogical(), &imgInfo, nullptr, &m_Image) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: failed to create depth image!");

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(m_Device->GetLogical(), m_Image, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReqs.size;
    allocInfo.memoryTypeIndex = Device::FindMemoryType(
        m_Device->GetPhysical(), memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_Device->GetLogical(), &allocInfo, nullptr, &m_Memory) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: failed to allocate depth image memory!");

    vkBindImageMemory(m_Device->GetLogical(), m_Image, m_Memory, 0);

    // Transition to DEPTH_STENCIL_READ_ONLY_OPTIMAL immediately so the first
    // frame can sample it before any shadow pass has run.
    TransitionLayout(cmdManager, depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

    // ----- Image view -----
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                           = m_Image;
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = depthFormat;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(m_Device->GetLogical(), &viewInfo, nullptr, &m_View) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: failed to create depth image view!");

    // ----- Comparison sampler -----
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter        = VK_FILTER_LINEAR;
    samplerInfo.minFilter        = VK_FILTER_LINEAR;
    samplerInfo.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // outside frustum = lit
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy    = 1.0f;
    samplerInfo.compareEnable    = VK_TRUE;
    samplerInfo.compareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
    samplerInfo.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod           = 0.0f;
    samplerInfo.maxLod           = 1.0f;

    if (vkCreateSampler(m_Device->GetLogical(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: failed to create comparison sampler!");

    // ----- Shadow render pass -----
    // The attachment stays in DEPTH_STENCIL_READ_ONLY_OPTIMAL from frame to frame.
    // The render pass handles the temporary transition to attachment-write via its
    // external dependencies (initialLayout == finalLayout == READ_ONLY_OPTIMAL).
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format         = depthFormat;
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 0;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 0;
    subpass.pDepthStencilAttachment = &depthRef;

    // Dependencies that handle READ_ONLY → WRITE and WRITE → READ_ONLY
    std::array<VkSubpassDependency, 2> deps{};

    deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass    = 0;
    deps[0].srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[0].dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    deps[1].srcSubpass    = 0;
    deps[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments    = &depthAttachment;
    rpInfo.subpassCount    = 1;
    rpInfo.pSubpasses      = &subpass;
    rpInfo.dependencyCount = static_cast<uint32_t>(deps.size());
    rpInfo.pDependencies   = deps.data();

    if (vkCreateRenderPass(m_Device->GetLogical(), &rpInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: failed to create render pass!");

    // ----- Framebuffer -----
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass      = m_RenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments    = &m_View;
    fbInfo.width           = SIZE;
    fbInfo.height          = SIZE;
    fbInfo.layers          = 1;

    if (vkCreateFramebuffer(m_Device->GetLogical(), &fbInfo, nullptr, &m_Framebuffer) != VK_SUCCESS)
        throw std::runtime_error("ShadowMap: failed to create framebuffer!");
}

// ---------------------------------------------------------------------------
void ShadowMap::Destroy()
{
    vkDestroyFramebuffer(m_Device->GetLogical(), m_Framebuffer, nullptr);
    vkDestroyRenderPass (m_Device->GetLogical(), m_RenderPass,  nullptr);
    vkDestroySampler    (m_Device->GetLogical(), m_Sampler,     nullptr);
    vkDestroyImageView  (m_Device->GetLogical(), m_View,        nullptr);
    vkFreeMemory        (m_Device->GetLogical(), m_Memory,      nullptr);
    vkDestroyImage      (m_Device->GetLogical(), m_Image,       nullptr);
}
