#version 450

layout(set = 1, binding = 0) uniform sampler2D albedoMap;


layout(set = 1, binding = 2) uniform MaterialUBO {
    vec4  baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float hasNormalMap;
    float hasMetallicRoughness;
    float hasAO;
    float hasEmissive;
    float alphaMask;
    float alphaCutoff;
} material;

layout(location = 0) in vec2 fragTexCoord;

void main()
{
    if (material.alphaMask > 0.5)
    {
        vec4 albedo = texture(albedoMap, fragTexCoord) * material.baseColorFactor;
        if (albedo.a < material.alphaCutoff)
            discard;
    }
}