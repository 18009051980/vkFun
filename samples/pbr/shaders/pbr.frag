#version 450

layout (binding = 0) uniform MatrixUbo
{
    vec3 viewPos;
    mat4 model;
    mat4 mvp;
    mat4 lightMvp;
} matrix;

layout (binding = 1) uniform Material
{
    vec3 albedo;
    float metallic;
    float roughness;
} material;

layout (binding = 2) uniform Light
{
    vec3 position;
    float intensity;
    vec3 color;
} light;

layout (binding = 3) uniform sampler2D shadowMap;


layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 outColor;

float PI = 3.14159265f;
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.f);
    float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.f;
    float k = (r * r) / 8.f;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float GemometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NDotV = max(dot(N, V), 0);
    float NdotL = max(dot(N, L), 0);
    float ggx1 = GeometrySchlickGGX(NDotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float shadowPCF(vec4 lightWorldPos, float bias)
{
    float shadow = 0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 tar = lightWorldPos.xy + vec2(x, y) * texelSize;
            float pcfDepth = texture(shadowMap, tar).r;
            shadow += (lightWorldPos.z - bias > pcfDepth) ? 0.0 : 1.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main()
{
    vec3 N = normalize(inNormal);
    vec3 V = normalize(matrix.viewPos.xyz - inPosition);
    vec3 L = normalize(light.position - inPosition);

    vec3 albedo = material.albedo;
    float metallic = material.metallic;
    float roughness = material.roughness;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0);
    vec3 H = normalize(V + L);
    float distance = length(light.position - inPosition);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = light.color * light.intensity;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GemometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);
    
    vec3 kS = F ;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.);
    vec3 diffuse = kD * albedo / PI;

    Lo += (diffuse + specular) * radiance * NdotL;
    
    vec4 lightWorldPos = matrix.lightMvp * vec4(inPosition, 1.0f);
    lightWorldPos = lightWorldPos / lightWorldPos.w;
    lightWorldPos.xy = lightWorldPos.xy * 0.5 + 0.5;
    
    float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005); // 根据法线和光线方向动态调整偏移量
    float shadow = shadowPCF(lightWorldPos, 0.0001);
    // float neareatPos = texture(shadowMap, lightWorldPos.xy).r;
    // shadow = (lightWorldPos.z - 0.00001) < neareatPos ? 1: 0;
    vec3 ambient = vec3(0.03) * albedo * 1;
    vec3 indirectLight = vec3(0.2) * albedo;
    vec3 color = ambient + Lo * shadow + indirectLight * (1 - shadow) * dot(N, L);

    outColor = vec4(color, 1.f);
}

