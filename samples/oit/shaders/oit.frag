#version 450
layout(early_fragment_tests) in;

struct Node
{
    uint next;
    float depth;
    float color[4];
};

layout (std430, binding = 1) buffer LinkedList
{
   Node nodes[];
};
layout (binding = 2, r32ui) uniform uimage2D headImage;
layout (std430, binding = 3) buffer Counter
{
    uint count;
    uint inMaxCount;
};

layout (location = 0) in vec4 fragColor;
layout (location = 0) out vec4 outColor;
void main()
{
    uint newNodeIndex = atomicAdd(count, 1);
    if (newNodeIndex >= inMaxCount)
    {
        discard;
    }
    uint oldhead = imageAtomicExchange(headImage, ivec2(gl_FragCoord.xy), newNodeIndex);
    nodes[newNodeIndex].next = oldhead;
    nodes[newNodeIndex].depth = gl_FragCoord.z;
    nodes[newNodeIndex].color[0] = fragColor[0];
    nodes[newNodeIndex].color[1] = fragColor[1];
    nodes[newNodeIndex].color[2] = fragColor[2];
    nodes[newNodeIndex].color[3] = fragColor[3];
    
}