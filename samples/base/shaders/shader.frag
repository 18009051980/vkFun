#version 450

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;


// Gamma校正参数（sRGB标准 gamma=2.2）
const float GAMMA = 2.2;
const float INV_GAMMA = 1.0 / GAMMA;

// 将sRGB颜色转换到线性空间
vec3 srgbToLinear(vec3 srgb) {
    return pow(srgb, vec3(GAMMA));
}
// 将线性颜色转换回sRGB空间
vec3 linearToSrgb(vec3 linear) {
    return pow(linear, vec3(INV_GAMMA));
}
void main() {
    vec2 newUV = vec2(fragTexCoord.x, 1 - fragTexCoord.y);
    outColor = texture(texSampler, newUV);
    outColor.rgb = linearToSrgb(outColor.rgb);
} 