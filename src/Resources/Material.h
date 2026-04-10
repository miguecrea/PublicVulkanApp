#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>

// Per-material PBR data loaded from glTF
struct Material
{
   
    VkImageView  albedoView = VK_NULL_HANDLE;
    VkSampler    albedoSampler = VK_NULL_HANDLE;
    VkImageView  normalView = VK_NULL_HANDLE;
    VkSampler    normalSampler = VK_NULL_HANDLE;
    VkImageView  metallicRoughnessView = VK_NULL_HANDLE;
    VkSampler    metallicRoughnessSampler = VK_NULL_HANDLE;
    VkImageView  aoView = VK_NULL_HANDLE;
    VkSampler    aoSampler = VK_NULL_HANDLE;
    VkImageView  emissiveView = VK_NULL_HANDLE;
    VkSampler    emissiveSampler = VK_NULL_HANDLE;

    // Factors
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float     metallicFactor = 0.0f;
    float     roughnessFactor = 0.5f;

    // Flags
    bool hasNormalMap = false;
    bool hasMetallicRoughness = false;
    bool hasAO = false;
    bool hasEmissive = false;
    bool doubleSided = false;
    bool alphaMask = false;
    float alphaCutoff = 0.5f;
};