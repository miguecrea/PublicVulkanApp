#include "Descriptors.h"
#include "../Core/Device.h"
#include "UniformBuffer.h"
#include <stdexcept>

// -------------------------------------------------------
// Camera layout � binding 0: UBO (vertex)
// -------------------------------------------------------
void Descriptors::CreateCameraLayout(Device* device)
{
    m_Device = device;

    VkDescriptorSetLayoutBinding ubo{};
    ubo.binding = 0;
    ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo.descriptorCount = 1;
    ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &ubo;

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &info, nullptr, &m_CameraLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create camera descriptor layout!");
}

void Descriptors::CreateCameraPool(Device* device, int framesInFlight)
{
    VkDescriptorPoolSize size{};
    size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    size.descriptorCount = framesInFlight;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes = &size;
    info.maxSets = framesInFlight;

    if (vkCreateDescriptorPool(device->GetLogical(), &info, nullptr, &m_CameraPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create camera descriptor pool!");
}

void Descriptors::CreateCameraSets(Device* device, int framesInFlight,
    const std::vector<VkBuffer>& cameraBuffers)
{
    std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_CameraLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_CameraPool;
    allocInfo.descriptorSetCount = framesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    m_CameraSets.resize(framesInFlight);
    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, m_CameraSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate camera descriptor sets!");

    for (int i = 0; i < framesInFlight; i++)
    {
        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = cameraBuffers[i];
        bufInfo.offset = 0;
        bufInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_CameraSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufInfo;

        vkUpdateDescriptorSets(device->GetLogical(), 1, &write, 0, nullptr);
    }
}

void Descriptors::DestroyCameraLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_CameraLayout, nullptr);
}

void Descriptors::DestroyCameraPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_CameraPool, nullptr);
}

// -------------------------------------------------------
// Material layout
// binding 0: albedo sampler (fragment)
// binding 1: normal sampler (fragment)
// binding 2: material UBO  (fragment)
// -------------------------------------------------------
void Descriptors::CreateMaterialLayout(Device* device)
{

    m_Device = device;

    std::array<VkDescriptorSetLayoutBinding, 6> bindings{};

    // binding 0: albedo
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 1: normal
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 2: material UBO
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 3: metallic/roughness
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 4: AO
    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 5: emissive
    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 6;
    info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &info, nullptr, &m_MaterialLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create material descriptor layout!");
   

}

void Descriptors::CreateMaterialPool(Device* device, int materialCount)
{
  
    std::array<VkDescriptorPoolSize, 2> sizes{};
    sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[0].descriptorCount = materialCount * 5; // albedo+normal+mr+ao+emissive
    sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[1].descriptorCount = materialCount;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 2;
    info.pPoolSizes = sizes.data();
    info.maxSets = materialCount;

    if (vkCreateDescriptorPool(device->GetLogical(), &info, nullptr, &m_MaterialPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create material descriptor pool!");
}

void Descriptors::CreateMaterialSets(Device* device, int materialCount,
    const std::vector<VkBuffer>& materialUBOs,
    const std::vector<VkImageView>& albedoViews,
    const std::vector<VkSampler>& albedoSamplers,
    const std::vector<VkImageView>& normalViews,
    const std::vector<VkSampler>& normalSamplers,
    const std::vector<VkImageView>& mrViews,
    const std::vector<VkSampler>& mrSamplers,
    const std::vector<VkImageView>& aoViews,
    const std::vector<VkSampler>& aoSamplers,
    const std::vector<VkImageView>& emissiveViews,
    const std::vector<VkSampler>& emissiveSamplers,
    VkImageView fallbackView, VkSampler fallbackSampler)
{
    std::vector<VkDescriptorSetLayout> layouts(materialCount, m_MaterialLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_MaterialPool;
    allocInfo.descriptorSetCount = materialCount;
    allocInfo.pSetLayouts = layouts.data();

    m_MaterialSets.resize(materialCount);
    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, m_MaterialSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate material descriptor sets!");

    for (int i = 0; i < materialCount; i++)
    {
        auto getView = [&](VkImageView v) { return v != VK_NULL_HANDLE ? v : fallbackView; };
        auto getSampler = [&](VkSampler s) { return s != VK_NULL_HANDLE ? s : fallbackSampler; };

        VkDescriptorImageInfo albedoInfo{};
        albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        albedoInfo.imageView = getView(albedoViews[i]);
        albedoInfo.sampler = getSampler(albedoSamplers[i]);

        VkDescriptorImageInfo normalInfo{};
        normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalInfo.imageView = getView(normalViews[i]);
        normalInfo.sampler = getSampler(normalSamplers[i]);

        VkDescriptorBufferInfo matInfo{};
        matInfo.buffer = materialUBOs[i];
        matInfo.offset = 0;
        matInfo.range = sizeof(MaterialUBO);

        VkDescriptorImageInfo mrInfo{};
        mrInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mrInfo.imageView = getView(mrViews[i]);
        mrInfo.sampler = getSampler(mrSamplers[i]);

        VkDescriptorImageInfo aoInfo{};
        aoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        aoInfo.imageView = getView(aoViews[i]);
        aoInfo.sampler = getSampler(aoSamplers[i]);

        VkDescriptorImageInfo emissiveInfo{};
        emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        emissiveInfo.imageView = getView(emissiveViews[i]);
        emissiveInfo.sampler = getSampler(emissiveSamplers[i]);

        std::array<VkWriteDescriptorSet, 6> writes{};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_MaterialSets[i];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &albedoInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_MaterialSets[i];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &normalInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = m_MaterialSets[i];
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &matInfo;

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].dstSet = m_MaterialSets[i];
        writes[3].dstBinding = 3;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[3].descriptorCount = 1;
        writes[3].pImageInfo = &mrInfo;

        writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[4].dstSet = m_MaterialSets[i];
        writes[4].dstBinding = 4;
        writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[4].descriptorCount = 1;
        writes[4].pImageInfo = &aoInfo;

        writes[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[5].dstSet = m_MaterialSets[i];
        writes[5].dstBinding = 5;
        writes[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[5].descriptorCount = 1;
        writes[5].pImageInfo = &emissiveInfo;

        vkUpdateDescriptorSets(device->GetLogical(), 6, writes.data(), 0, nullptr);
    }
}

void Descriptors::DestroyMaterialLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_MaterialLayout, nullptr);
}

void Descriptors::DestroyMaterialPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_MaterialPool, nullptr);
}

// -------------------------------------------------------
// Lighting layout
// binding 0-3: input attachments
// binding 4: light UBO (per frame)
// -------------------------------------------------------
void Descriptors::CreateLightingLayout(Device* device)
{
    m_Device = device;

    std::array<VkDescriptorSetLayoutBinding, 10> bindings{};

    // bindings 0-4: G-Buffer input attachments
    for (int i = 0; i < 5; i++)
    {
        bindings[i].binding         = i;
        bindings[i].descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    // binding 5: light UBO
    bindings[5].binding         = 5;
    bindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 6: shadow map (combined image sampler with comparison)
    bindings[6].binding         = 6;
    bindings[6].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[6].descriptorCount = 1;
    bindings[6].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 7: irradiance cubemap
    bindings[7].binding         = 7;
    bindings[7].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[7].descriptorCount = 1;
    bindings[7].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 8: prefiltered specular environment map
    bindings[8].binding         = 8;
    bindings[8].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[8].descriptorCount = 1;
    bindings[8].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    // binding 9: BRDF integration LUT
    bindings[9].binding         = 9;
    bindings[9].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[9].descriptorCount = 1;
    bindings[9].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 10;
    info.pBindings    = bindings.data();

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &info, nullptr, &m_LightingLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create lighting descriptor layout!");
}

void Descriptors::CreateLightingPool(Device* device)
{
    std::array<VkDescriptorPoolSize, 3> sizes{};
    sizes[0].type            = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    sizes[0].descriptorCount = 5 * 2; // 5 G-Buffer attachments * 2 frames
    sizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[1].descriptorCount = 2;     // light UBO * 2 frames
    sizes[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sizes[2].descriptorCount = 8;     // shadow(6)+irradiance(7)+prefilter(8)+brdfLut(9), 2 frames each

    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = static_cast<uint32_t>(sizes.size());
    info.pPoolSizes    = sizes.data();
    info.maxSets       = 2; // one per frame

    if (vkCreateDescriptorPool(device->GetLogical(), &info, nullptr, &m_LightingPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create lighting descriptor pool!");
}

void Descriptors::CreateLightingSet(Device* device,
    const std::array<VkImageView, 5>& gbufferViews,
    const std::vector<VkBuffer>& lightBuffers, int framesInFlight,
    VkImageView shadowView,    VkSampler shadowSampler,
    VkImageView irrView,       VkSampler irrSampler,
    VkImageView prefilterView, VkSampler prefilterSampler,
    VkImageView brdfLutView,   VkSampler brdfLutSampler)
{
    std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_LightingLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = m_LightingPool;
    allocInfo.descriptorSetCount = framesInFlight;
    allocInfo.pSetLayouts        = layouts.data();

    m_LightingSets.resize(framesInFlight);
    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, m_LightingSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate lighting descriptor sets!");

    for (int f = 0; f < framesInFlight; f++)
    {
        std::array<VkDescriptorImageInfo, 5> gbufInfos{};
        std::array<VkWriteDescriptorSet, 10> writes{};

        // bindings 0-4: G-Buffer input attachments
        for (int i = 0; i < 5; i++)
        {
            gbufInfos[i].imageView   = gbufferViews[i];
            gbufInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            writes[i].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet          = m_LightingSets[f];
            writes[i].dstBinding      = i;
            writes[i].descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            writes[i].descriptorCount = 1;
            writes[i].pImageInfo      = &gbufInfos[i];
        }

        // binding 5: light UBO
        VkDescriptorBufferInfo lightInfo{};
        lightInfo.buffer = lightBuffers[f];
        lightInfo.offset = 0;
        lightInfo.range  = sizeof(LightUBO);

        writes[5].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[5].dstSet          = m_LightingSets[f];
        writes[5].dstBinding      = 5;
        writes[5].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[5].descriptorCount = 1;
        writes[5].pBufferInfo     = &lightInfo;

        // binding 6: shadow map (depth, comparison sampler)
        VkDescriptorImageInfo shadowInfo{};
        shadowInfo.imageView   = shadowView;
        shadowInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        shadowInfo.sampler     = shadowSampler;

        writes[6].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[6].dstSet          = m_LightingSets[f];
        writes[6].dstBinding      = 6;
        writes[6].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[6].descriptorCount = 1;
        writes[6].pImageInfo      = &shadowInfo;

        // binding 7: irradiance cubemap
        VkDescriptorImageInfo irrInfo{};
        irrInfo.imageView   = irrView;
        irrInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        irrInfo.sampler     = irrSampler;

        writes[7].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[7].dstSet          = m_LightingSets[f];
        writes[7].dstBinding      = 7;
        writes[7].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[7].descriptorCount = 1;
        writes[7].pImageInfo      = &irrInfo;

        // binding 8: prefiltered specular env map
        VkDescriptorImageInfo pfInfo{};
        pfInfo.imageView   = prefilterView;
        pfInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pfInfo.sampler     = prefilterSampler;

        writes[8].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[8].dstSet          = m_LightingSets[f];
        writes[8].dstBinding      = 8;
        writes[8].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[8].descriptorCount = 1;
        writes[8].pImageInfo      = &pfInfo;

        // binding 9: BRDF integration LUT
        VkDescriptorImageInfo lutInfo{};
        lutInfo.imageView   = brdfLutView;
        lutInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        lutInfo.sampler     = brdfLutSampler;

        writes[9].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[9].dstSet          = m_LightingSets[f];
        writes[9].dstBinding      = 9;
        writes[9].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[9].descriptorCount = 1;
        writes[9].pImageInfo      = &lutInfo;

        vkUpdateDescriptorSets(device->GetLogical(), 10, writes.data(), 0, nullptr);
    }
}

void Descriptors::DestroyLightingLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_LightingLayout, nullptr);
}

void Descriptors::DestroyLightingPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_LightingPool, nullptr);
    m_LightingSets.clear();
}

void Descriptors::CreateToneMappingLayout(Device* device)
{

    m_Device = device;

    // binding 0: HDR input attachment
    // binding 1: light UBO (for exposure settings)
    std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 2;
    info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device->GetLogical(), &info, nullptr, &m_ToneMappingLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create tone mapping descriptor layout!");
}

void Descriptors::CreateToneMappingPool(Device* device)
{

    std::array<VkDescriptorPoolSize, 2> sizes{};
    sizes[0].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    sizes[0].descriptorCount = 2;
    sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[1].descriptorCount = 2;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 2;
    info.pPoolSizes = sizes.data();
    info.maxSets = 2;

    if (vkCreateDescriptorPool(device->GetLogical(), &info, nullptr, &m_ToneMappingPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create tone mapping descriptor pool!");
}

void Descriptors::CreateToneMappingSet(Device* device, VkImageView hdrView,
    const std::vector<VkBuffer>& lightBuffers, int framesInFlight)
{
    std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_ToneMappingLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_ToneMappingPool;
    allocInfo.descriptorSetCount = framesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    m_ToneMappingSets.resize(framesInFlight);
    if (vkAllocateDescriptorSets(device->GetLogical(), &allocInfo, m_ToneMappingSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate tone mapping descriptor sets!");

    for (int f = 0; f < framesInFlight; f++)
    {
        VkDescriptorImageInfo hdrInfo{};
        hdrInfo.imageView = hdrView;
        hdrInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorBufferInfo lightInfo{};
        lightInfo.buffer = lightBuffers[f];
        lightInfo.offset = 0;
        lightInfo.range = sizeof(LightUBO);

        std::array<VkWriteDescriptorSet, 2> writes{};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_ToneMappingSets[f];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &hdrInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_ToneMappingSets[f];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo = &lightInfo;

        vkUpdateDescriptorSets(device->GetLogical(), 2, writes.data(), 0, nullptr);
    }
}


void Descriptors::DestroyToneMappingLayout()
{
    vkDestroyDescriptorSetLayout(m_Device->GetLogical(), m_ToneMappingLayout, nullptr);
}

void Descriptors::DestroyToneMappingPool()
{
    vkDestroyDescriptorPool(m_Device->GetLogical(), m_ToneMappingPool, nullptr);
    m_ToneMappingSets.clear();
}