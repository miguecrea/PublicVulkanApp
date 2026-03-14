#version 450

layout(binding = 1) uniform sampler2D albedoMap;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outMetallicRoughness;

void main()
{
    outPosition          = vec4(fragWorldPos, 1.0);
    outNormal = vec4(fragNormal, 0.0); // no normalize
    outAlbedo            = texture(albedoMap, fragTexCoord);
    outMetallicRoughness = vec4(0.0, 0.5, 0.0, 0.0);
}