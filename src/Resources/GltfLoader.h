#pragma once
#include <string>
#include <vector>
#include "Mesh.h"
#include "Material.h"
#include "Texture.h"
#include <vulkan/vulkan.h>

class Device;
class CommandManager;

struct GltfMesh
{
    Mesh     mesh;
    int      materialIndex = -1;
};

struct GltfScene
{
    std::vector<GltfMesh> meshes;
    std::vector<Material> materials;

    // All loaded textures - owned here, materials just reference views/samplers
    std::vector<VkImage>        images;
    std::vector<VkDeviceMemory> memories;
    std::vector<VkImageView>    views;
    std::vector<VkSampler>      samplers;

    void Destroy(VkDevice device);
};

class GltfLoader
{
public:
    static GltfScene Load(
        const std::string& path,
        Device* device,
        CommandManager* cmdManager
    );
};