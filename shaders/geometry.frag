#version 450


layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform MaterialUBO {
    vec4  baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float hasNormalMap;
    float padding;
} material;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 fragTBN;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outMetallicRoughness;

void main()
{
    outPosition = vec4(fragWorldPos, 1.0);

    // Normal mapping
    vec3 N;
    if (material.hasNormalMap > 0.5)
    {
        vec3 tangentNormal = texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0;
        N = normalize(fragTBN * tangentNormal);
    }
    else
    {
        N = normalize(fragTBN[2]); // use geometry normal (TBN[2] = N)
    }
    outNormal = vec4(N * 0.5 + 0.5, 0.0); // encode to [0,1]

    outAlbedo            = texture(albedoMap, fragTexCoord) * material.baseColorFactor;
    outMetallicRoughness = vec4(material.metallicFactor, material.roughnessFactor, 0.0, 0.0);
}