#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // vec2 newUV = vec2(fragTexCoord.x, 1 - fragTexCoord.y);
    // outColor = vec4(fragTexCoord, 0.f, 1.f);
    outColor = vec4(fragColor, 1.f);
} 