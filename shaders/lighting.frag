#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inPosition;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inNormal;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inAlbedo;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput inMetallicRoughness;

layout(location = 0) out vec4 outColor;

const vec3  LIGHT_DIR   = normalize(vec3(1.0, -1.0, 1.0));
const vec3  LIGHT_COLOR = vec3(0.0, 0.0,1.0);
const float AMBIENT     = 1.5;

void main()
{
    vec3 worldPos = subpassLoad(inPosition).rgb;
    vec3 normal   = normalize(subpassLoad(inNormal).rgb);
    vec4 albedo   = subpassLoad(inAlbedo);

    float NdotL  = max(dot(normal, LIGHT_DIR), 0.0);
    vec3 diffuse = LIGHT_COLOR * NdotL;
    vec3 ambient = vec3(AMBIENT);
    vec3 color   = albedo.rgb * (ambient + diffuse);


    outColor = vec4(color, 1.0);
}