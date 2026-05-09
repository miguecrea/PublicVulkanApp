#include "GltfLoader.h"
#include "../Core/Device.h"
#include "../Core/CommandManager.h"
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_map>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#include <tiny_gltf.h>
#include "Buffer.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

// -------------------------------------------------------
// GltfScene cleanup
// -------------------------------------------------------
void GltfScene::Destroy(VkDevice device)
{
    for (auto s : samplers) vkDestroySampler(device, s, nullptr);
    for (auto v : views)    vkDestroyImageView(device, v, nullptr);
    for (int i = 0; i < (int)images.size(); i++)
    {
        vkDestroyImage(device, images[i], nullptr);
        vkFreeMemory(device, memories[i], nullptr);
    }

}

// -------------------------------------------------------
// Helpers
// -------------------------------------------------------
static VkSampler CreateDefaultSampler(Device* device)
{
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(device->GetPhysical(), &props);

    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy = props.limits.maxSamplerAnisotropy;
    info.minLod = 0.0f;
    info.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler sampler;
    if (vkCreateSampler(device->GetLogical(), &info, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create sampler!");
    return sampler;
}

static void UploadTexture(
    tinygltf::Model& model, int texIndex, bool srgb,
    Device* device, CommandManager* cmdManager,
    GltfScene& scene,
    std::unordered_map<int, std::pair<VkImageView, VkSampler>>& cache,
    VkImageView& outView, VkSampler& outSampler)
{
    auto& tex = model.textures[texIndex];
    auto& img = model.images[tex.source];

    if (img.image.empty())
    {
        outView = VK_NULL_HANDLE;
        outSampler = VK_NULL_HANDLE;
        return;
    }

    // Key encodes both source index and sRGB flag to handle the same image
    // used with different formats (e.g., albedo sRGB vs. normal linear).
    int cacheKey = tex.source * 2 + (srgb ? 1 : 0);
    auto it = cache.find(cacheKey);
    if (it != cache.end())
    {
        outView    = it->second.first;
        outSampler = it->second.second;
        return;
    }

    VkFormat format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    uint32_t mipLevels = static_cast<uint32_t>(
        std::floor(std::log2(std::max(img.width, img.height)))) + 1;

    // Create staging buffer
    VkDeviceSize imageSize = img.width * img.height * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    Buffer::CreateBuffer(device, imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(device->GetLogical(), stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, img.image.data(), imageSize);
    vkUnmapMemory(device->GetLogical(), stagingMemory);

    // Image with mip chain; TRANSFER_SRC is required for vkCmdBlitImage downsampling.
    VkImage       vkImage;
    VkDeviceMemory vkMemory;
    Texture::CreateImage(device, img.width, img.height, format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vkImage, vkMemory, mipLevels);

    Texture::TransitionImageLayout(device, cmdManager, vkImage, format,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

    // Copy staging into mip 0
    VkCommandBuffer cmd = cmdManager->BeginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { (uint32_t)img.width, (uint32_t)img.height, 1 };
    vkCmdCopyBufferToImage(cmd, stagingBuffer, vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    cmdManager->EndSingleTimeCommands(cmd);

    // Build remaining mips and transition all to SHADER_READ_ONLY_OPTIMAL
    Texture::GenerateMipmaps(device, cmdManager, vkImage, format,
        img.width, img.height, mipLevels);

    vkDestroyBuffer(device->GetLogical(), stagingBuffer, nullptr);
    vkFreeMemory(device->GetLogical(), stagingMemory, nullptr);

    VkImageView view = Texture::CreateImageView(device, vkImage, format,
        VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    VkSampler sampler = CreateDefaultSampler(device);

    scene.images.push_back(vkImage);
    scene.memories.push_back(vkMemory);
    scene.views.push_back(view);
    scene.samplers.push_back(sampler);

    cache[cacheKey] = {view, sampler};
    outView    = view;
    outSampler = sampler;
}

// -------------------------------------------------------
// Main loader
// -------------------------------------------------------
GltfScene GltfLoader::Load(
    const std::string& path,
    Device* device,
    CommandManager* cmdManager)
{
    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    std::string warn, err;

    bool ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!warn.empty()) std::cout << "glTF warning: " << warn << "\n";
    if (!ok) throw std::runtime_error("Failed to load glTF: " + err);

    GltfScene scene;
    std::unordered_map<int, std::pair<VkImageView, VkSampler>> texCache;

    // -------------------------------------------------------
    // Load materials
    // -------------------------------------------------------
    for (auto& mat : model.materials)
    {
        Material m{};
        auto& pbr = mat.pbrMetallicRoughness;

        // Base color factor
        if (pbr.baseColorFactor.size() == 4)
            m.baseColorFactor = glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1],
                pbr.baseColorFactor[2], pbr.baseColorFactor[3]);

        m.metallicFactor = static_cast<float>(pbr.metallicFactor);
        m.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
        if (mat.emissiveFactor.size() == 3)
            m.emissiveFactor = glm::vec3(mat.emissiveFactor[0],
                                         mat.emissiveFactor[1],
                                         mat.emissiveFactor[2]);
        m.doubleSided = mat.doubleSided;
        m.alphaMask = (mat.alphaMode == "MASK");
        m.alphaCutoff = static_cast<float>(mat.alphaCutoff);

        // Albedo texture (sRGB)
        if (pbr.baseColorTexture.index >= 0)
            UploadTexture(model, pbr.baseColorTexture.index, true,
                device, cmdManager, scene, texCache, m.albedoView, m.albedoSampler);

        // Normal map (linear)
        if (mat.normalTexture.index >= 0)
        {
            UploadTexture(model, mat.normalTexture.index, false,
                device, cmdManager, scene, texCache, m.normalView, m.normalSampler);
            m.hasNormalMap = (m.normalView != VK_NULL_HANDLE);
        }

        if (pbr.metallicRoughnessTexture.index >= 0)
        {
            UploadTexture(model, pbr.metallicRoughnessTexture.index, false,
                device, cmdManager, scene, texCache,
                m.metallicRoughnessView, m.metallicRoughnessSampler);
            m.hasMetallicRoughness = (m.metallicRoughnessView != VK_NULL_HANDLE);
        }

        // AO (linear)
        if (mat.occlusionTexture.index >= 0)
        {
            UploadTexture(model, mat.occlusionTexture.index, false,
                device, cmdManager, scene, texCache,
                m.aoView, m.aoSampler);
            m.hasAO = (m.aoView != VK_NULL_HANDLE);
        }

        // Emissive (sRGB)
        if (mat.emissiveTexture.index >= 0)
        {
            UploadTexture(model, mat.emissiveTexture.index, true,
                device, cmdManager, scene, texCache,
                m.emissiveView, m.emissiveSampler);
            m.hasEmissive = (m.emissiveView != VK_NULL_HANDLE);
        }

        scene.materials.push_back(m);
    }

    // -------------------------------------------------------
    // Load meshes — traverse scene node hierarchy so that
    // per-node TRS transforms are baked into vertex data.
    // -------------------------------------------------------
    auto processPrimitive = [&](const tinygltf::Primitive& prim,
                                const glm::mat4& worldTransform)
    {
        if (prim.mode != TINYGLTF_MODE_TRIANGLES) return;
        if (prim.attributes.find("POSITION") == prim.attributes.end()) return;

        GltfMesh gltfMesh{};
        gltfMesh.materialIndex = prim.material;

        glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(worldTransform)));

        auto getBuffer = [&](const std::string& attr) -> const float*
        {
            auto it = prim.attributes.find(attr);
            if (it == prim.attributes.end()) return nullptr;
            auto& acc  = model.accessors[it->second];
            auto& view = model.bufferViews[acc.bufferView];
            auto& buf  = model.buffers[view.buffer];
            return reinterpret_cast<const float*>(
                buf.data.data() + view.byteOffset + acc.byteOffset);
        };

        const float* positions = getBuffer("POSITION");
        const float* normals   = getBuffer("NORMAL");
        const float* texcoords = getBuffer("TEXCOORD_0");
        const float* tangents  = getBuffer("TANGENT");

        auto& posAcc   = model.accessors[prim.attributes.at("POSITION")];
        size_t vertCount = posAcc.count;

        gltfMesh.mesh.vertices.resize(vertCount);
        for (size_t i = 0; i < vertCount; i++)
        {
            Vertex& v = gltfMesh.mesh.vertices[i];

            glm::vec3 localPos(positions[i * 3 + 0],
                               positions[i * 3 + 1],
                               positions[i * 3 + 2]);
            v.pos = glm::vec3(worldTransform * glm::vec4(localPos, 1.0f));
            v.color = { 1.0f, 1.0f, 1.0f };

            glm::vec3 localNormal = normals
                ? glm::vec3(normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2])
                : glm::vec3(0.0f, 0.0f, 1.0f);
            v.normal = glm::normalize(normalMatrix * localNormal);

            v.texCoord = texcoords
                ? glm::vec2(texcoords[i * 2 + 0], texcoords[i * 2 + 1])
                : glm::vec2(0.0f);

            if (tangents)
            {
                glm::vec3 localTan(tangents[i * 4 + 0],
                                   tangents[i * 4 + 1],
                                   tangents[i * 4 + 2]);
                glm::vec3 worldTan = glm::normalize(normalMatrix * localTan);
                v.tangent = glm::vec4(worldTan, tangents[i * 4 + 3]); // preserve handedness
            }
            else
            {
                v.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        // Indices
        auto& idxAcc  = model.accessors[prim.indices];
        auto& idxView = model.bufferViews[idxAcc.bufferView];
        auto& idxBuf  = model.buffers[idxView.buffer];
        const uint8_t* idxData = idxBuf.data.data() + idxView.byteOffset + idxAcc.byteOffset;

        gltfMesh.mesh.indices.resize(idxAcc.count);
        for (size_t i = 0; i < idxAcc.count; i++)
        {
            if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                gltfMesh.mesh.indices[i] = reinterpret_cast<const uint16_t*>(idxData)[i];
            else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                gltfMesh.mesh.indices[i] = reinterpret_cast<const uint32_t*>(idxData)[i];
            else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                gltfMesh.mesh.indices[i] = idxData[i];
        }

        scene.meshes.push_back(gltfMesh);
    };

    // Recursive node traversal — accumulates parent * local transform
    std::function<void(int, const glm::mat4&)> processNode =
        [&](int nodeIdx, const glm::mat4& parentTransform)
    {
        const tinygltf::Node& node = model.nodes[nodeIdx];

        glm::mat4 localTransform(1.0f);
        if (!node.matrix.empty())
        {
            // glTF matrix is column-major, matching GLM's layout
            localTransform = glm::make_mat4(node.matrix.data());
        }
        else
        {
            glm::mat4 T(1.0f), R(1.0f), S(1.0f);
            if (!node.translation.empty())
                T = glm::translate(glm::mat4(1.0f),
                    glm::vec3(node.translation[0],
                              node.translation[1],
                              node.translation[2]));
            if (!node.rotation.empty())
            {
                // glTF quaternion order: [x, y, z, w]
                glm::quat q(static_cast<float>(node.rotation[3]),
                            static_cast<float>(node.rotation[0]),
                            static_cast<float>(node.rotation[1]),
                            static_cast<float>(node.rotation[2]));
                R = glm::mat4_cast(q);
            }
            if (!node.scale.empty())
                S = glm::scale(glm::mat4(1.0f),
                    glm::vec3(node.scale[0], node.scale[1], node.scale[2]));

            localTransform = T * R * S;
        }

        glm::mat4 worldTransform = parentTransform * localTransform;

        if (node.mesh >= 0)
        {
            const tinygltf::Mesh& gMesh = model.meshes[node.mesh];
            for (const auto& prim : gMesh.primitives)
                processPrimitive(prim, worldTransform);
        }

        for (int childIdx : node.children)
            processNode(childIdx, worldTransform);
    };

    // Start from every root node in the default scene
    int sceneIdx = model.defaultScene >= 0 ? model.defaultScene : 0;
    if (!model.scenes.empty())
    {
        for (int rootNode : model.scenes[sceneIdx].nodes)
            processNode(rootNode, glm::mat4(1.0f));
    }

    std::cout << "Loaded glTF: " << scene.meshes.size() << " meshes, "
        << scene.materials.size() << " materials\n";
    return scene;
}