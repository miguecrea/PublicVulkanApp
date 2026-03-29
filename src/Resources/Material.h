#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>

// Per-material PBR data loaded from glTF
struct Material
{
    // Texture views + samplers (VK_NULL_HANDLE if not present)
    VkImageView  albedoView = VK_NULL_HANDLE;
    VkSampler    albedoSampler = VK_NULL_HANDLE;
    VkImageView  normalView = VK_NULL_HANDLE;
    VkSampler    normalSampler = VK_NULL_HANDLE;

    // Factors (used when no texture is present)
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float     metallicFactor = 0.0f;
    float     roughnessFactor = 0.5f;

    bool hasNormalMap = false;
    bool doubleSided = false;
    bool alphaMask = false;
    float alphaCutoff = 0.5f;
};