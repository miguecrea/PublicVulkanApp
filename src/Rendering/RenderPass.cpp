#include "RenderPass.h"
#include "GBuffer.h"
#include "../Core/Device.h"
#include <stdexcept>
#include <array>




// Attachment indices (must match framebuffer attachment order)
// 0: Swapchain color
// 1: Depth
// 2: Position (G-Buffer)
// 3: Normal   (G-Buffer)
// 4: Albedo   (G-Buffer)
// 5: MetallicRoughness (G-Buffer)

void RenderPass::Create(Device* device, VkFormat colorFormat)
{
    m_Device = device;
    VkFormat depthFormat = FindDepthFormat();

    // -------------------------------------------------------
    // Attachment descriptions
    // -------------------------------------------------------
    std::array<VkAttachmentDescription, 6> attachments{};

    // 0 - Swapchain color (final output)
    attachments[0].format = colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // 1 - Depth (written in prepass, read in geometry pass)
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 2-5: G-Buffer attachments (written in geometry, read as input in lighting)
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
    // Subpass 1 — Geometry Pass (fill G-Buffer)
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
    // Subpass 2 — Lighting Pass (read G-Buffer, write final color)
    // -------------------------------------------------------
    VkAttachmentReference finalColorRef{};
    finalColorRef.attachment = 0;
    finalColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference, 4> inputRefs{};
    for (int i = 0; i < 4; i++)
    {
        inputRefs[i].attachment = 2 + i;
        inputRefs[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkSubpassDescription subpass2{};
    subpass2.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass2.colorAttachmentCount = 1;
    subpass2.pColorAttachments = &finalColorRef;
    subpass2.inputAttachmentCount = 4;
    subpass2.pInputAttachments = inputRefs.data();

    // -------------------------------------------------------
    // Subpass dependencies
    // -------------------------------------------------------
    std::array<VkSubpassDependency, 4> deps{};

    // External -> Subpass 0 (depth clear + write)
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[0].srcAccessMask = 0;
    deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Subpass 0 -> Subpass 1 (depth write -> depth read EQUAL)
    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = 1;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    // Subpass 1 -> Subpass 2 (G-Buffer write -> input attachment read)
    deps[2].srcSubpass = 1;
    deps[2].dstSubpass = 2;
    deps[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

    // Subpass 2 -> External (final color -> present)
    deps[3].srcSubpass = 2;
    deps[3].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    deps[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[3].dstAccessMask = 0;

    // -------------------------------------------------------
    // Create render pass
    // -------------------------------------------------------
    std::array<VkSubpassDescription, 3> subpasses = { subpass0, subpass1, subpass2 };

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