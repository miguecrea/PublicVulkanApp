#include "IBL.h"
#include "../Core/Device.h"
#include "../Core/CommandManager.h"
#include <stb_image.h>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <array>
#include <algorithm>

static uint32_t FindMemType(VkPhysicalDevice physical, uint32_t filter,
                             VkMemoryPropertyFlags props)
{
    return Device::FindMemoryType(physical, filter, props);
}

static std::vector<char> ReadSPV(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("IBL: cannot open shader: " + path);
    size_t size = (size_t)file.tellg();
    std::vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), size);
    return buf;
}

static VkShaderModule MakeModule(VkDevice device, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode    = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule mod;
    if (vkCreateShaderModule(device, &info, nullptr, &mod) != VK_SUCCESS)
        throw std::runtime_error("IBL: failed to create shader module!");
    return mod;
}

static std::string ShaderPath(const std::string& name)
{
    return std::string(PROJECT_SOURCE_DIR) + "/ShadersOutput/" + name + ".spv";
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

bool IBL::Create(Device* device, CommandManager* cmdManager, const std::string& hdrPath)
{
    m_Device = device;

    int w = 0, h = 0, channels = 0;
    float* pixels = stbi_loadf(hdrPath.c_str(), &w, &h, &channels, 4);
    if (!pixels)
    {
        CreateFallback(cmdManager);
        return false;
    }

    VkDeviceSize imageBytes = (VkDeviceSize)w * h * 4 * sizeof(float);

    VkBuffer       stagingBuf;
    VkDeviceMemory stagingMem;
    {
        VkBufferCreateInfo bufInfo{};
        bufInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size        = imageBytes;
        bufInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(m_Device->GetLogical(), &bufInfo, nullptr, &stagingBuf) != VK_SUCCESS)
            throw std::runtime_error("IBL: failed to create staging buffer!");

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(m_Device->GetLogical(), stagingBuf, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReqs.size;
        allocInfo.memoryTypeIndex = FindMemType(m_Device->GetPhysical(),
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(m_Device->GetLogical(), &allocInfo, nullptr, &stagingMem) != VK_SUCCESS)
            throw std::runtime_error("IBL: failed to allocate staging memory!");

        vkBindBufferMemory(m_Device->GetLogical(), stagingBuf, stagingMem, 0);
        void* data;
        vkMapMemory(m_Device->GetLogical(), stagingMem, 0, imageBytes, 0, &data);
        memcpy(data, pixels, imageBytes);
        vkUnmapMemory(m_Device->GetLogical(), stagingMem);
    }
    stbi_image_free(pixels);

    VkImage        equirectImage;
    VkDeviceMemory equirectMemory;
    {
        VkImageCreateInfo imgInfo{};
        imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgInfo.imageType     = VK_IMAGE_TYPE_2D;
        imgInfo.format        = VK_FORMAT_R32G32B32A32_SFLOAT;
        imgInfo.extent        = { (uint32_t)w, (uint32_t)h, 1 };
        imgInfo.mipLevels     = 1;
        imgInfo.arrayLayers   = 1;
        imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imgInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(m_Device->GetLogical(), &imgInfo, nullptr, &equirectImage) != VK_SUCCESS)
            throw std::runtime_error("IBL: failed to create equirect image!");

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(m_Device->GetLogical(), equirectImage, &memReqs);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReqs.size;
        allocInfo.memoryTypeIndex = FindMemType(m_Device->GetPhysical(),
            memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(m_Device->GetLogical(), &allocInfo, nullptr, &equirectMemory) != VK_SUCCESS)
            throw std::runtime_error("IBL: failed to allocate equirect image memory!");
        vkBindImageMemory(m_Device->GetLogical(), equirectImage, equirectMemory, 0);
    }

    {
        VkCommandBuffer cmd = cmdManager->BeginSingleTimeCommands();

        auto barrier = [&](VkImageLayout oldL, VkImageLayout newL,
                           VkAccessFlags srcA, VkAccessFlags dstA,
                           VkPipelineStageFlags src, VkPipelineStageFlags dst)
        {
            VkImageMemoryBarrier b{};
            b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.oldLayout           = oldL;
            b.newLayout           = newL;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image               = equirectImage;
            b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
            b.srcAccessMask       = srcA;
            b.dstAccessMask       = dstA;
            vkCmdPipelineBarrier(cmd, src, dst, 0, 0, nullptr, 0, nullptr, 1, &b);
        };

        barrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy copy{};
        copy.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        copy.imageExtent      = { (uint32_t)w, (uint32_t)h, 1 };
        vkCmdCopyBufferToImage(cmd, stagingBuf, equirectImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        barrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        cmdManager->EndSingleTimeCommands(cmd);
    }
    vkFreeMemory(m_Device->GetLogical(), stagingMem, nullptr);
    vkDestroyBuffer(m_Device->GetLogical(), stagingBuf, nullptr);

    VkImageView equirectView;
    VkSampler   equirectSampler;
    {
        constexpr VkFormat cubeFmt = VK_FORMAT_R16G16B16A16_SFLOAT;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image            = equirectImage;
        viewInfo.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format           = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        if (vkCreateImageView(m_Device->GetLogical(), &viewInfo, nullptr, &equirectView) != VK_SUCCESS)
            throw std::runtime_error("IBL: failed to create equirect image view!");

        VkSamplerCreateInfo sampInfo{};
        sampInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampInfo.magFilter    = VK_FILTER_LINEAR;
        sampInfo.minFilter    = VK_FILTER_LINEAR;
        sampInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        if (vkCreateSampler(m_Device->GetLogical(), &sampInfo, nullptr, &equirectSampler) != VK_SUCCESS)
            throw std::runtime_error("IBL: failed to create equirect sampler!");

        constexpr VkFormat cf = VK_FORMAT_R16G16B16A16_SFLOAT;
        CreateCubemapImage(ENV_SIZE, cf, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            m_EnvCube, m_EnvMemory);
        CreateCubemapImage(IRR_SIZE, cf, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            m_Irradiance, m_IrrMemory);

        m_EnvView        = CreateCubeView(m_EnvCube,    cf);
        m_IrradianceView = CreateCubeView(m_Irradiance, cf);

        auto makeSampler = [&]() -> VkSampler {
            VkSamplerCreateInfo s{};
            s.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            s.magFilter    = VK_FILTER_LINEAR;
            s.minFilter    = VK_FILTER_LINEAR;
            s.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            s.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            s.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            VkSampler sam;
            vkCreateSampler(m_Device->GetLogical(), &s, nullptr, &sam);
            return sam;
        };
        m_EnvSampler        = makeSampler();
        m_IrradianceSampler = makeSampler();
    }

    RunPrecompute(cmdManager, equirectImage, equirectView, equirectSampler);

    vkDestroySampler  (m_Device->GetLogical(), equirectSampler, nullptr);
    vkDestroyImageView(m_Device->GetLogical(), equirectView,    nullptr);
    vkFreeMemory      (m_Device->GetLogical(), equirectMemory,  nullptr);
    vkDestroyImage    (m_Device->GetLogical(), equirectImage,   nullptr);

    // Specular IBL: prefiltered env map + BRDF LUT (runs while env cube still exists)
    RunSpecularPrecompute(cmdManager);

    // Env cube is no longer needed after all IBL precomputation
    vkDestroySampler  (m_Device->GetLogical(), m_EnvSampler, nullptr);
    vkDestroyImageView(m_Device->GetLogical(), m_EnvView,    nullptr);
    vkFreeMemory      (m_Device->GetLogical(), m_EnvMemory,  nullptr);
    vkDestroyImage    (m_Device->GetLogical(), m_EnvCube,    nullptr);
    m_EnvSampler = VK_NULL_HANDLE;
    m_EnvView    = VK_NULL_HANDLE;
    m_EnvMemory  = VK_NULL_HANDLE;
    m_EnvCube    = VK_NULL_HANDLE;

    m_Valid = true;
    return true;
}

// ---------------------------------------------------------------------------
void IBL::RunPrecompute(CommandManager* cmdManager,
    VkImage equirectImage, VkImageView equirectView, VkSampler equirectSampler)
{
    constexpr VkFormat cubeFmt = VK_FORMAT_R16G16B16A16_SFLOAT;

    VkDescriptorSetLayout compLayout;
    {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        bindings[0] = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
        bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 2;
        info.pBindings    = bindings.data();
        vkCreateDescriptorSetLayout(m_Device->GetLogical(), &info, nullptr, &compLayout);
    }

    VkPipelineLayout compPipelineLayout;
    {
        VkPushConstantRange pc{};
        pc.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pc.offset     = 0;
        pc.size       = sizeof(int);

        VkPipelineLayoutCreateInfo info{};
        info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount         = 1;
        info.pSetLayouts            = &compLayout;
        info.pushConstantRangeCount = 1;
        info.pPushConstantRanges    = &pc;
        vkCreatePipelineLayout(m_Device->GetLogical(), &info, nullptr, &compPipelineLayout);
    }

    VkDescriptorPool compPool;
    {
        std::array<VkDescriptorPoolSize, 2> sizes{};
        sizes[0] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12 };
        sizes[1] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 12 };
        VkDescriptorPoolCreateInfo info{};
        info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets       = 12;
        info.poolSizeCount = 2;
        info.pPoolSizes    = sizes.data();
        vkCreateDescriptorPool(m_Device->GetLogical(), &info, nullptr, &compPool);
    }

    VkPipeline equirectPipeline, irrPipeline;
    {
        auto makeCompPipeline = [&](const std::vector<char>& code) -> VkPipeline {
            VkShaderModule mod = MakeModule(m_Device->GetLogical(), code);
            VkComputePipelineCreateInfo info{};
            info.sType        = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            info.stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            info.stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
            info.stage.module = mod;
            info.stage.pName  = "main";
            info.layout       = compPipelineLayout;
            VkPipeline pipe;
            vkCreateComputePipelines(m_Device->GetLogical(), VK_NULL_HANDLE, 1, &info, nullptr, &pipe);
            vkDestroyShaderModule(m_Device->GetLogical(), mod, nullptr);
            return pipe;
        };
        equirectPipeline = makeCompPipeline(ReadSPV(ShaderPath("equirect_to_cube.comp")));
        irrPipeline      = makeCompPipeline(ReadSPV(ShaderPath("irradiance_conv.comp")));
    }

    auto makeSet = [&](VkImageView samplerView, VkSampler sampler, VkImageLayout samplerLayout,
                       VkImageView storageView) -> VkDescriptorSet
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = compPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &compLayout;
        VkDescriptorSet set;
        vkAllocateDescriptorSets(m_Device->GetLogical(), &allocInfo, &set);

        VkDescriptorImageInfo sampInfo{ sampler, samplerView, samplerLayout };
        VkDescriptorImageInfo storInfo{ VK_NULL_HANDLE, storageView, VK_IMAGE_LAYOUT_GENERAL };

        std::array<VkWriteDescriptorSet, 2> writes{};
        writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, 0, 0, 1,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &sampInfo };
        writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, 1, 0, 1,
                      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storInfo };
        vkUpdateDescriptorSets(m_Device->GetLogical(), 2, writes.data(), 0, nullptr);
        return set;
    };

    std::array<VkImageView, 6> envFaceViews{};
    std::array<VkImageView, 6> irrFaceViews{};
    for (uint32_t f = 0; f < 6; f++)
    {
        envFaceViews[f] = CreateFaceView(m_EnvCube,    cubeFmt, f);
        irrFaceViews[f] = CreateFaceView(m_Irradiance, cubeFmt, f);
    }

    VkCommandBuffer cmd = cmdManager->BeginSingleTimeCommands();

    BarrierCubemap(cmd, m_EnvCube,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, VK_ACCESS_SHADER_WRITE_BIT);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, equirectPipeline);
    for (int face = 0; face < 6; face++)
    {
        VkDescriptorSet set = makeSet(equirectView, equirectSampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, envFaceViews[face]);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            compPipelineLayout, 0, 1, &set, 0, nullptr);
        vkCmdPushConstants(cmd, compPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
            0, sizeof(int), &face);
        uint32_t groups = (ENV_SIZE + 15) / 16;
        vkCmdDispatch(cmd, groups, groups, 1);
    }

    BarrierCubemap(cmd, m_EnvCube,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    BarrierCubemap(cmd, m_Irradiance,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, VK_ACCESS_SHADER_WRITE_BIT);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, irrPipeline);
    for (int face = 0; face < 6; face++)
    {
        VkDescriptorSet set = makeSet(m_EnvView, m_EnvSampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, irrFaceViews[face]);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
            compPipelineLayout, 0, 1, &set, 0, nullptr);
        vkCmdPushConstants(cmd, compPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
            0, sizeof(int), &face);
        uint32_t groups = (IRR_SIZE + 7) / 8;
        vkCmdDispatch(cmd, groups, groups, 1);
    }

    BarrierCubemap(cmd, m_Irradiance,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

    cmdManager->EndSingleTimeCommands(cmd);

    for (uint32_t f = 0; f < 6; f++)
    {
        vkDestroyImageView(m_Device->GetLogical(), envFaceViews[f], nullptr);
        vkDestroyImageView(m_Device->GetLogical(), irrFaceViews[f], nullptr);
    }

    vkDestroyDescriptorPool     (m_Device->GetLogical(), compPool,           nullptr);
    vkDestroyPipelineLayout     (m_Device->GetLogical(), compPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), compLayout,         nullptr);
    vkDestroyPipeline           (m_Device->GetLogical(), equirectPipeline,   nullptr);
    vkDestroyPipeline           (m_Device->GetLogical(), irrPipeline,        nullptr);
}

// ---------------------------------------------------------------------------
void IBL::RunSpecularPrecompute(CommandManager* cmdManager)
{
    constexpr VkFormat cubeFmt = VK_FORMAT_R16G16B16A16_SFLOAT;
    constexpr VkFormat lutFmt  = VK_FORMAT_R16G16_SFLOAT;

    // Create prefiltered env cubemap with multiple mip levels
    CreateCubemapImage(PREFILTER_SIZE, cubeFmt,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        m_Prefilter, m_PrefilterMemory, PREFILTER_MIP_LEVELS);

    m_PrefilterView = CreateCubeView(m_Prefilter, cubeFmt, PREFILTER_MIP_LEVELS);

    {
        VkSamplerCreateInfo s{};
        s.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        s.magFilter        = VK_FILTER_LINEAR;
        s.minFilter        = VK_FILTER_LINEAR;
        s.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        s.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        s.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        s.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        s.minLod           = 0.0f;
        s.maxLod           = static_cast<float>(PREFILTER_MIP_LEVELS);
        vkCreateSampler(m_Device->GetLogical(), &s, nullptr, &m_PrefilterSampler);
    }

    // Create BRDF LUT image
    CreateImage2D(BRDF_LUT_SIZE, BRDF_LUT_SIZE, lutFmt,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        m_BrdfLUT, m_BrdfLUTMemory);

    m_BrdfLUTView = Create2DView(m_BrdfLUT, lutFmt);

    {
        VkSamplerCreateInfo s{};
        s.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        s.magFilter    = VK_FILTER_LINEAR;
        s.minFilter    = VK_FILTER_LINEAR;
        s.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        s.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        s.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(m_Device->GetLogical(), &s, nullptr, &m_BrdfLUTSampler);
    }

    // ---- Descriptor layouts ----
    // Layout A (for prefilter): binding 0 = samplerCube, binding 1 = storage image
    VkDescriptorSetLayout layoutA;
    {
        std::array<VkDescriptorSetLayoutBinding, 2> b{};
        b[0] = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
        b[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
        VkDescriptorSetLayoutCreateInfo info{};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 2;
        info.pBindings    = b.data();
        vkCreateDescriptorSetLayout(m_Device->GetLogical(), &info, nullptr, &layoutA);
    }

    // Layout B (for BRDF LUT): binding 0 = storage image only
    VkDescriptorSetLayout layoutB;
    {
        VkDescriptorSetLayoutBinding b{ 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
        VkDescriptorSetLayoutCreateInfo info{};
        info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings    = &b;
        vkCreateDescriptorSetLayout(m_Device->GetLogical(), &info, nullptr, &layoutB);
    }

    // ---- Pipeline layouts ----
    // Prefilter push constant: int face + float roughness = 8 bytes
    VkPipelineLayout layoutPfPipe;
    {
        VkPushConstantRange pc{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int) + sizeof(float) };
        VkPipelineLayoutCreateInfo info{};
        info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount         = 1;
        info.pSetLayouts            = &layoutA;
        info.pushConstantRangeCount = 1;
        info.pPushConstantRanges    = &pc;
        vkCreatePipelineLayout(m_Device->GetLogical(), &info, nullptr, &layoutPfPipe);
    }

    // BRDF LUT pipeline layout: no push constants
    VkPipelineLayout layoutLutPipe;
    {
        VkPipelineLayoutCreateInfo info{};
        info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount = 1;
        info.pSetLayouts    = &layoutB;
        vkCreatePipelineLayout(m_Device->GetLogical(), &info, nullptr, &layoutLutPipe);
    }

    // ---- Descriptor pool ----
    // PREFILTER_MIP_LEVELS * 6 prefilter sets + 1 LUT set
    const uint32_t pfSets = PREFILTER_MIP_LEVELS * 6;
    VkDescriptorPool pool;
    {
        std::array<VkDescriptorPoolSize, 2> sizes{};
        sizes[0] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pfSets };
        sizes[1] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, pfSets + 1 };
        VkDescriptorPoolCreateInfo info{};
        info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets       = pfSets + 1;
        info.poolSizeCount = 2;
        info.pPoolSizes    = sizes.data();
        vkCreateDescriptorPool(m_Device->GetLogical(), &info, nullptr, &pool);
    }

    // ---- Compute pipelines ----
    auto makeCompPipeline = [&](const std::vector<char>& code, VkPipelineLayout layout) -> VkPipeline {
        VkShaderModule mod = MakeModule(m_Device->GetLogical(), code);
        VkComputePipelineCreateInfo info{};
        info.sType        = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        info.stage.module = mod;
        info.stage.pName  = "main";
        info.layout       = layout;
        VkPipeline pipe;
        vkCreateComputePipelines(m_Device->GetLogical(), VK_NULL_HANDLE, 1, &info, nullptr, &pipe);
        vkDestroyShaderModule(m_Device->GetLogical(), mod, nullptr);
        return pipe;
    };

    VkPipeline pfPipeline  = makeCompPipeline(ReadSPV(ShaderPath("prefilter_env.comp")),  layoutPfPipe);
    VkPipeline lutPipeline = makeCompPipeline(ReadSPV(ShaderPath("brdf_lut.comp")), layoutLutPipe);

    // ---- Per-face-per-mip storage views ----
    std::vector<VkImageView> pfFaceViews(pfSets);
    for (uint32_t mip = 0; mip < PREFILTER_MIP_LEVELS; mip++)
        for (uint32_t face = 0; face < 6; face++)
            pfFaceViews[mip * 6 + face] = CreateFaceView(m_Prefilter, cubeFmt, face, mip);

    // BRDF LUT storage view
    VkImageView lutStorageView = Create2DView(m_BrdfLUT, lutFmt);

    // ---- Helper: allocate and write prefilter descriptor set ----
    auto makePfSet = [&](VkImageView storageView) -> VkDescriptorSet
    {
        VkDescriptorSetAllocateInfo ai{};
        ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool     = pool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts        = &layoutA;
        VkDescriptorSet set;
        vkAllocateDescriptorSets(m_Device->GetLogical(), &ai, &set);

        VkDescriptorImageInfo envInfo{ m_EnvSampler, m_EnvView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkDescriptorImageInfo storInfo{ VK_NULL_HANDLE, storageView, VK_IMAGE_LAYOUT_GENERAL };

        std::array<VkWriteDescriptorSet, 2> writes{};
        writes[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, 0, 0, 1,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &envInfo };
        writes[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, set, 1, 0, 1,
                      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storInfo };
        vkUpdateDescriptorSets(m_Device->GetLogical(), 2, writes.data(), 0, nullptr);
        return set;
    };

    // BRDF LUT descriptor set
    VkDescriptorSet lutSet;
    {
        VkDescriptorSetAllocateInfo ai{};
        ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool     = pool;
        ai.descriptorSetCount = 1;
        ai.pSetLayouts        = &layoutB;
        vkAllocateDescriptorSets(m_Device->GetLogical(), &ai, &lutSet);

        VkDescriptorImageInfo storInfo{ VK_NULL_HANDLE, lutStorageView, VK_IMAGE_LAYOUT_GENERAL };
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, lutSet,
            0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &storInfo };
        vkUpdateDescriptorSets(m_Device->GetLogical(), 1, &write, 0, nullptr);
    }

    // ---- Submit all compute work ----
    VkCommandBuffer cmd = cmdManager->BeginSingleTimeCommands();

    // Transition prefilter cubemap (all mips, all faces) to GENERAL for storage writes
    {
        VkImageMemoryBarrier b{};
        b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image               = m_Prefilter;
        b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, PREFILTER_MIP_LEVELS, 0, 6 };
        b.srcAccessMask       = 0;
        b.dstAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &b);
    }

    // Transition BRDF LUT to GENERAL
    {
        VkImageMemoryBarrier b{};
        b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image               = m_BrdfLUT;
        b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        b.srcAccessMask       = 0;
        b.dstAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &b);
    }

    // Prefilter: iterate mips then faces
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pfPipeline);
    for (uint32_t mip = 0; mip < PREFILTER_MIP_LEVELS; mip++)
    {
        float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVELS - 1);
        uint32_t mipSize = PREFILTER_SIZE >> mip;

        for (int face = 0; face < 6; face++)
        {
            VkDescriptorSet set = makePfSet(pfFaceViews[mip * 6 + face]);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                layoutPfPipe, 0, 1, &set, 0, nullptr);

            struct PfPC { int face; float roughness; } pc{ face, roughness };
            vkCmdPushConstants(cmd, layoutPfPipe, VK_SHADER_STAGE_COMPUTE_BIT,
                0, sizeof(PfPC), &pc);

            uint32_t groups = std::max(1u, (mipSize + 7) / 8);
            vkCmdDispatch(cmd, groups, groups, 1);
        }
    }

    // BRDF LUT dispatch
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, lutPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        layoutLutPipe, 0, 1, &lutSet, 0, nullptr);
    {
        uint32_t groups = (BRDF_LUT_SIZE + 15) / 16;
        vkCmdDispatch(cmd, groups, groups, 1);
    }

    // Transition prefilter to SHADER_READ_ONLY
    {
        VkImageMemoryBarrier b{};
        b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
        b.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image               = m_Prefilter;
        b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, PREFILTER_MIP_LEVELS, 0, 6 };
        b.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
        b.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &b);
    }

    // Transition BRDF LUT to SHADER_READ_ONLY
    {
        VkImageMemoryBarrier b{};
        b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout           = VK_IMAGE_LAYOUT_GENERAL;
        b.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image               = m_BrdfLUT;
        b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        b.srcAccessMask       = VK_ACCESS_SHADER_WRITE_BIT;
        b.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &b);
    }

    cmdManager->EndSingleTimeCommands(cmd);

    // Cleanup per-face views and temporary resources
    for (auto& v : pfFaceViews) vkDestroyImageView(m_Device->GetLogical(), v, nullptr);
    vkDestroyImageView      (m_Device->GetLogical(), lutStorageView, nullptr);
    vkDestroyDescriptorPool (m_Device->GetLogical(), pool,           nullptr);
    vkDestroyPipeline       (m_Device->GetLogical(), pfPipeline,     nullptr);
    vkDestroyPipeline       (m_Device->GetLogical(), lutPipeline,    nullptr);
    vkDestroyPipelineLayout (m_Device->GetLogical(), layoutPfPipe,   nullptr);
    vkDestroyPipelineLayout (m_Device->GetLogical(), layoutLutPipe,  nullptr);
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), layoutA,    nullptr);
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), layoutB,    nullptr);
}

// ---------------------------------------------------------------------------
void IBL::CreateFallback(CommandManager* cmdManager)
{
    constexpr VkFormat cubeFmt = VK_FORMAT_R16G16B16A16_SFLOAT;
    constexpr VkFormat lutFmt  = VK_FORMAT_R16G16_SFLOAT;

    // 1×1 white irradiance cubemap
    CreateCubemapImage(1, cubeFmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        m_Irradiance, m_IrrMemory);

    // 1×1 white prefilter cubemap (1 mip)
    CreateCubemapImage(1, cubeFmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        m_Prefilter, m_PrefilterMemory);

    // 1×1 BRDF LUT (0.5, 0.5)
    CreateImage2D(1, 1, lutFmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        m_BrdfLUT, m_BrdfLUTMemory);

    // Upload white pixels for irradiance and prefilter cubemaps (6 faces each)
    constexpr uint16_t one = 0x3C00; // 1.0 in half float
    constexpr uint16_t half = 0x3800; // 0.5 in half float

    // Irradiance: 6 faces × 4 channels
    const uint16_t irrPixels[6 * 4] = {
        one,one,one,one, one,one,one,one, one,one,one,one,
        one,one,one,one, one,one,one,one, one,one,one,one
    };
    // Prefilter: same
    const uint16_t pfPixels[6 * 4] = {
        one,one,one,one, one,one,one,one, one,one,one,one,
        one,one,one,one, one,one,one,one, one,one,one,one
    };
    // BRDF LUT: 1 pixel, 2 channels (R=0.5, G=0.5)
    const uint16_t lutPixels[2] = { half, half };

    auto uploadToImage = [&](VkImage image, const void* data, VkDeviceSize bytes,
                              uint32_t layers, VkExtent3D extent)
    {
        VkBuffer       stageBuf;
        VkDeviceMemory stageMem;
        {
            VkBufferCreateInfo bi{};
            bi.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bi.size        = bytes;
            bi.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(m_Device->GetLogical(), &bi, nullptr, &stageBuf);

            VkMemoryRequirements mr;
            vkGetBufferMemoryRequirements(m_Device->GetLogical(), stageBuf, &mr);
            VkMemoryAllocateInfo ai{};
            ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize  = mr.size;
            ai.memoryTypeIndex = FindMemType(m_Device->GetPhysical(), mr.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            vkAllocateMemory(m_Device->GetLogical(), &ai, nullptr, &stageMem);
            vkBindBufferMemory(m_Device->GetLogical(), stageBuf, stageMem, 0);

            void* dst;
            vkMapMemory(m_Device->GetLogical(), stageMem, 0, bytes, 0, &dst);
            memcpy(dst, data, bytes);
            vkUnmapMemory(m_Device->GetLogical(), stageMem);
        }

        VkCommandBuffer cmd = cmdManager->BeginSingleTimeCommands();

        VkImageMemoryBarrier b{};
        b.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        b.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image               = image;
        b.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers };
        b.srcAccessMask       = 0;
        b.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &b);

        VkDeviceSize perLayerBytes = bytes / layers;
        std::vector<VkBufferImageCopy> copies(layers);
        for (uint32_t lay = 0; lay < layers; lay++)
        {
            copies[lay].bufferOffset      = lay * perLayerBytes;
            copies[lay].bufferRowLength   = 0;
            copies[lay].bufferImageHeight = 0;
            copies[lay].imageSubresource  = { VK_IMAGE_ASPECT_COLOR_BIT, 0, lay, 1 };
            copies[lay].imageExtent       = extent;
        }
        vkCmdCopyBufferToImage(cmd, stageBuf, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layers, copies.data());

        b.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &b);

        cmdManager->EndSingleTimeCommands(cmd);
        vkFreeMemory  (m_Device->GetLogical(), stageMem, nullptr);
        vkDestroyBuffer(m_Device->GetLogical(), stageBuf, nullptr);
    };

    uploadToImage(m_Irradiance, irrPixels, sizeof(irrPixels), 6, { 1, 1, 1 });
    uploadToImage(m_Prefilter,  pfPixels,  sizeof(pfPixels),  6, { 1, 1, 1 });
    uploadToImage(m_BrdfLUT,    lutPixels, sizeof(lutPixels),  1, { 1, 1, 1 });

    m_IrradianceView = CreateCubeView(m_Irradiance, cubeFmt);
    m_PrefilterView  = CreateCubeView(m_Prefilter,  cubeFmt);
    m_BrdfLUTView    = Create2DView  (m_BrdfLUT,    lutFmt);

    auto makeSampler = [&]() -> VkSampler {
        VkSamplerCreateInfo s{};
        s.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        s.magFilter    = VK_FILTER_LINEAR;
        s.minFilter    = VK_FILTER_LINEAR;
        s.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        s.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        s.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        VkSampler sam;
        vkCreateSampler(m_Device->GetLogical(), &s, nullptr, &sam);
        return sam;
    };
    m_IrradianceSampler = makeSampler();
    m_PrefilterSampler  = makeSampler();
    m_BrdfLUTSampler    = makeSampler();
}

// ---------------------------------------------------------------------------
void IBL::Destroy()
{
    if (m_BrdfLUTSampler)    vkDestroySampler  (m_Device->GetLogical(), m_BrdfLUTSampler,    nullptr);
    if (m_BrdfLUTView)       vkDestroyImageView(m_Device->GetLogical(), m_BrdfLUTView,       nullptr);
    if (m_BrdfLUTMemory)     vkFreeMemory       (m_Device->GetLogical(), m_BrdfLUTMemory,     nullptr);
    if (m_BrdfLUT)           vkDestroyImage     (m_Device->GetLogical(), m_BrdfLUT,           nullptr);
    if (m_PrefilterSampler)  vkDestroySampler  (m_Device->GetLogical(), m_PrefilterSampler,  nullptr);
    if (m_PrefilterView)     vkDestroyImageView(m_Device->GetLogical(), m_PrefilterView,     nullptr);
    if (m_PrefilterMemory)   vkFreeMemory       (m_Device->GetLogical(), m_PrefilterMemory,   nullptr);
    if (m_Prefilter)         vkDestroyImage     (m_Device->GetLogical(), m_Prefilter,         nullptr);
    if (m_IrradianceSampler) vkDestroySampler  (m_Device->GetLogical(), m_IrradianceSampler, nullptr);
    if (m_IrradianceView)    vkDestroyImageView(m_Device->GetLogical(), m_IrradianceView,    nullptr);
    if (m_IrrMemory)         vkFreeMemory       (m_Device->GetLogical(), m_IrrMemory,         nullptr);
    if (m_Irradiance)        vkDestroyImage     (m_Device->GetLogical(), m_Irradiance,        nullptr);
}

// ---------------------------------------------------------------------------
// Internal image helpers
// ---------------------------------------------------------------------------
void IBL::CreateCubemapImage(uint32_t size, VkFormat format, VkImageUsageFlags usage,
                              VkImage& outImage, VkDeviceMemory& outMemory, uint32_t mipLevels)
{
    VkImageCreateInfo imgInfo{};
    imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imgInfo.imageType     = VK_IMAGE_TYPE_2D;
    imgInfo.format        = format;
    imgInfo.extent        = { size, size, 1 };
    imgInfo.mipLevels     = mipLevels;
    imgInfo.arrayLayers   = 6;
    imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage         = usage;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(m_Device->GetLogical(), &imgInfo, nullptr, &outImage) != VK_SUCCESS)
        throw std::runtime_error("IBL: failed to create cubemap image!");

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(m_Device->GetLogical(), outImage, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemType(m_Device->GetPhysical(),
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_Device->GetLogical(), &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
        throw std::runtime_error("IBL: failed to allocate cubemap memory!");

    vkBindImageMemory(m_Device->GetLogical(), outImage, outMemory, 0);
}

void IBL::CreateImage2D(uint32_t width, uint32_t height, VkFormat format,
                         VkImageUsageFlags usage, VkImage& outImage, VkDeviceMemory& outMemory)
{
    VkImageCreateInfo imgInfo{};
    imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType     = VK_IMAGE_TYPE_2D;
    imgInfo.format        = format;
    imgInfo.extent        = { width, height, 1 };
    imgInfo.mipLevels     = 1;
    imgInfo.arrayLayers   = 1;
    imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage         = usage;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(m_Device->GetLogical(), &imgInfo, nullptr, &outImage) != VK_SUCCESS)
        throw std::runtime_error("IBL: failed to create 2D image!");

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(m_Device->GetLogical(), outImage, &memReqs);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemType(m_Device->GetPhysical(),
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_Device->GetLogical(), &allocInfo, nullptr, &outMemory) != VK_SUCCESS)
        throw std::runtime_error("IBL: failed to allocate 2D image memory!");

    vkBindImageMemory(m_Device->GetLogical(), outImage, outMemory, 0);
}

VkImageView IBL::CreateCubeView(VkImage image, VkFormat format, uint32_t mipLevels)
{
    VkImageViewCreateInfo info{};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = image;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_CUBE;
    info.format                          = format;
    info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = mipLevels;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 6;
    VkImageView view;
    if (vkCreateImageView(m_Device->GetLogical(), &info, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("IBL: failed to create cube image view!");
    return view;
}

VkImageView IBL::CreateFaceView(VkImage image, VkFormat format, uint32_t face, uint32_t mip)
{
    VkImageViewCreateInfo info{};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = image;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.format                          = format;
    info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel   = mip;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = face;
    info.subresourceRange.layerCount     = 1;
    VkImageView view;
    if (vkCreateImageView(m_Device->GetLogical(), &info, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("IBL: failed to create face image view!");
    return view;
}

VkImageView IBL::Create2DView(VkImage image, VkFormat format)
{
    VkImageViewCreateInfo info{};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = image;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.format                          = format;
    info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;
    VkImageView view;
    if (vkCreateImageView(m_Device->GetLogical(), &info, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("IBL: failed to create 2D image view!");
    return view;
}

void IBL::BarrierCubemap(VkCommandBuffer cmd, VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
    VkAccessFlags srcAccess, VkAccessFlags dstAccess, uint32_t mipLevels)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 6 };
    barrier.srcAccessMask       = srcAccess;
    barrier.dstAccessMask       = dstAccess;
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
        0, nullptr, 0, nullptr, 1, &barrier);
}
