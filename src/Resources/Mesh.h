#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color
            && texCoord == other.texCoord && normal == other.normal;
    }

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription desc{};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> descs{};

        descs[0].binding = 0; descs[0].location = 0;
        descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[0].offset = offsetof(Vertex, pos);

        descs[1].binding = 0; descs[1].location = 1;
        descs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[1].offset = offsetof(Vertex, color);

        descs[2].binding = 0; descs[2].location = 2;
        descs[2].format = VK_FORMAT_R32G32_SFLOAT;
        descs[2].offset = offsetof(Vertex, texCoord);

        descs[3].binding = 0; descs[3].location = 3;
        descs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        descs[3].offset = offsetof(Vertex, normal);

        return descs;
    }
};

// Hash for Vertex - needed for deduplication during model loading
namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& v) const
        {
            size_t h1 = hash<glm::vec3>()(v.pos);
            size_t h2 = hash<glm::vec3>()(v.color);
            size_t h3 = hash<glm::vec2>()(v.texCoord);
            size_t h4 = hash<glm::vec3>()(v.normal);
            return ((h1 ^ (h2 << 1)) >> 1) ^ (h3 << 1) ^ (h4 << 2);
        }
    };
}

struct Mesh
{
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
};
