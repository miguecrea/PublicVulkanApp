#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inHDR;

layout(binding = 1) uniform LightUBO {
    vec4 dirLightDir;
    vec4 dirLightColor;
    vec4 camPos;
    // Physical camera settings
    float aperture;
    float shutterSpeed;
    float iso;
    float padding;
} lights;

layout(location = 0) out vec4 outColor;

// -------------------------------------------------------
// Physical Camera Exposure
// -------------------------------------------------------
float CalculateEV100(float aperture, float shutterTime, float ISO)
{
    return log2((aperture * aperture) / shutterTime * 100.0 / ISO);
}

float ConvertEV100ToExposure(float EV100)
{
    float maxLuminance = 1.2 * pow(2.0, EV100);
    return 1.0 / max(maxLuminance, 0.0001);
}

// -------------------------------------------------------
// Tone Mapping Operators
// -------------------------------------------------------
vec3 ReinhardToneMapping(vec3 color)
{
    return color / (color + vec3(1.0));
}

vec3 ACESFilmToneMapping(vec3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 Uncharted2ToneMappingCurve(vec3 color)
{
    const float a = 0.15;
    const float b = 0.50;
    const float c = 0.10;
    const float d = 0.20;
    const float e = 0.02;
    const float f = 0.30;
    return ((color * (a * color + c * b) + d * e) / (color * (a * color + b) + d * f)) - e / f;
}

vec3 Uncharted2ToneMapping(vec3 color)
{
    const float W = 11.2;
    vec3 curvedColor = Uncharted2ToneMappingCurve(color);
    float whiteScale = 1.0 / Uncharted2ToneMappingCurve(vec3(W)).r;
    return clamp(curvedColor * whiteScale, 0.0, 1.0);
}

// -------------------------------------------------------
void main()
{
  

    vec3 hdrColor = subpassLoad(inHDR).rgb;

    // Physical camera exposure
    float ev100 = CalculateEV100(lights.aperture, lights.shutterSpeed, lights.iso);
    float exposure = ConvertEV100ToExposure(ev100);
    hdrColor *= exposure;

    vec3 ldrColor = ACESFilmToneMapping(hdrColor);
    outColor = vec4(ldrColor, 1.0);

}