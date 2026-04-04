#include "RenderPass.h"
#include "GBuffer.h"
#include "HDRBuffer.h"
#include "../Core/Device.h"
#include <stdexcept>
#include <array>

void RenderPass::Create(Device* device, VkFormat colorFormat)
{
    m_Device = device;
    VkFormat depthFormat = FindDepthFormat();

    // -------------------------------------------------------
    // 7 attachments
    // -------------------------------------------------------
    std::array<VkAttachmentDescription, 7> attachments{};

    // 0 - Swapchain
    attachments[0].format = colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // 1 - Depth
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 2-5 - GBuffer
    for (int i = 0; i < GBuffer::Count; i++)
    {
        attachments[2 + i].format = GBuffer::Formats[i];
        attachments[2 + i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[2 + i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[2 + i].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2 + i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2 + i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2 + i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[2 + i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // 6 - HDR image
    attachments[6].format = HDRBuffer::Format;
    attachments[6].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[6].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[6].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[6].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[6].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[6].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[6].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // -------------------------------------------------------
    // Subpass 0 — Depth Prepass
    // -------------------------------------------------------
    VkAttachmentReference depthWriteRef{};
    depthWriteRef.attachment = 1;
    depthWriteRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass0{};
    subpass0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass0.colorAttachmentCount = 0;
    subpass0.pDepthStencilAttachment = &depthWriteRef;

    // -------------------------------------------------------
    // Subpass 1 — Geometry Pass
    // -------------------------------------------------------
    std::array<VkAttachmentReference, 4> gbufferColorRefs{};
    for (int i = 0; i < 4; i++)
    {
        gbufferColorRefs[i].attachment = 2 + i;
        gbufferColorRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference depthReadRef{};
    depthReadRef.attachment = 1;
    depthReadRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass1{};
    subpass1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass1.colorAttachmentCount = 4;
    subpass1.pColorAttachments = gbufferColorRefs.data();
    subpass1.pDepthStencilAttachment = &depthReadRef;

    // -------------------------------------------------------
    // Subpass 2 — Lighting Pass (writes to HDR image)
    // -------------------------------------------------------
    VkAttachmentReference hdrColorRef{};
    hdrColorRef.attachment = 6;
    hdrColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 4> gbufferInputRefs{};
    for (int i = 0; i < 4; i++)
    {
        gbufferInputRefs[i].attachment = 2 + i;
        gbufferInputRefs[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkSubpassDescription subpass2{};
    subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass2.colorAttachmentCount = 1;
    subpass2.pColorAttachments = &hdrColorRef;
    subpass2.inputAttachmentCount = 4;
    subpass2.pInputAttachments = gbufferInputRefs.data();

    // -------------------------------------------------------
    // Subpass 3 — Tone Mapping (reads HDR, writes to swapchain)
    // -------------------------------------------------------
    VkAttachmentReference swapchainColorRef{};
    swapchainColorRef.attachment = 0;
    swapchainColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference hdrInputRef{};
    hdrInputRef.attachment = 6;
    hdrInputRef.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkSubpassDescription subpass3{};
    subpass3.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass3.colorAttachmentCount = 1;
    subpass3.pColorAttachments = &swapchainColorRef;
    subpass3.inputAttachmentCount = 1;
    subpass3.pInputAttachments = &hdrInputRef;

    // -------------------------------------------------------
    // Dependencies
    // -------------------------------------------------------
    std::array<VkSubpassDependency, 5> deps{};

    // External -> Subpass 0
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[0].srcAccessMask = 0;
    deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Subpass 0 -> Subpass 1
    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = 1;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    // Subpass 1 -> Subpass 2 (GBuffer write -> lighting read)
    deps[2].srcSubpass = 1;
    deps[2].dstSubpass = 2;
    deps[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

    // Subpass 2 -> Subpass 3 (HDR write -> tone mapping read)
    deps[3].srcSubpass = 2;
    deps[3].dstSubpass = 3;
    deps[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[3].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

    // Subpass 3 -> External
    deps[4].srcSubpass = 3;
    deps[4].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[4].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    deps[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[4].dstAccessMask = 0;

    // -------------------------------------------------------
    // Create
    // -------------------------------------------------------
    std::array<VkSubpassDescription, 4> subpasses = { subpass0, subpass1, subpass2, subpass3 };

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpInfo.pAttachments = attachments.data();
    rpInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    rpInfo.pSubpasses = subpasses.data();
    rpInfo.dependencyCount = static_cast<uint32_t>(deps.size());
    rpInfo.pDependencies = deps.data();

    if (vkCreateRenderPass(m_Device->GetLogical(), &rpInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create deferred render pass!");
}

void RenderPass::Destroy()
{
    vkDestroyRenderPass(m_Device->GetLogical(), m_RenderPass, nullptr);
}

VkFormat RenderPass::FindDepthFormat()
{
    const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Device->GetPhysical(), format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return format;
    }
    throw std::runtime_error("Failed to find supported depth format!");
}