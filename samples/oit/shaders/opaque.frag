#version 450


layout (binding = 1) uniform sampler2D texSampler;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outColor;

void main(){
    vec2 newUV = vec2(inTexCoord.x, 1 - inTexCoord.y);
    outColor = texture(texSampler, newUV);
}

