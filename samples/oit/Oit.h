#pragma once

#include <GlfwApp.h>


#include <ImageLoader.h>
#include <ObjLoader.h>

using MyVertex = Vertex<static_cast<VertexAttributeBits>(std::to_underlying(VertexAttributeBits::POSITION) | std::to_underlying(VertexAttributeBits::COLOR) | std::to_underlying(VertexAttributeBits::TEXCOORD) | std::to_underlying(VertexAttributeBits::NORMAL))>;
class Oit : public GlfwApp
{
public:
    void init() override;
    void clean() override;
    void drawFrame() override;

    void createFramebuffers() override;
    void cleanupFramebuffers() override;

    void cleanupDepthImage() override;
    void createDepthImage() override;

    void cleanupColorResources() override;
    void createColorResources() override;
private:
    void createDescriptorSetLayout();
    void createPipelineLayout();

    void createBlendComputePipeline();
    void createOpaqueGraphichPipeline();
    void createOitGraphicsPipeline();
    void createRenderPass();

    void allocateCommandBuffer();

    void createSamplerImage();

    void createVertexBuffer(VkBuffer&buffer, VkDeviceMemory&memory, const std::vector<MyVertex> & vertices);
    void createIndexBuffer(VkBuffer&buffer, VkDeviceMemory&memory, const std::vector<uint32_t>& indices);

    void createUniformBuffer();
    void updateUniformBuffer();


    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void recordComputeCmdBuffer(uint32_t imageIndex);

    void allocateCommandBuffer(VkCommandBuffer &commandBuffer);

    void allocateDescriptorSets();
    
    void updateDescriptorSets();
private:
    constexpr inline static uint32_t m_framesInFlight = 2;

    VkDescriptorSetLayout m_opaqueDescriptorSetLayout;
    VkPipelineLayout m_opaquePipelineLayout;

    VkDescriptorSetLayout m_oitDescriptorSetLayout;
    VkPipelineLayout m_oitPipelineLayout;

    VkDescriptorSetLayout m_blendDescriptorSetLayout;
    VkPipelineLayout m_blendPipelineLayout;
    

    VkPipeline m_opaquePipeline;
    VkPipeline m_oitPipeline;
    VkPipeline m_blendPipeline;

    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;
    
    std::vector<VkFramebuffer> m_framebuffers;

    VkRenderPass m_renderPass;

    VkImage m_vikingroomImage;
    VkDeviceMemory m_vikingroomImageMemory;
    VkImageView m_vikingroomImageView;

    VkSampler m_textureSampler;

    std::vector<MyVertex> m_vertices;
    std::vector<uint32_t> m_indices;

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;

    std::vector<MyVertex> m_oitVertices;
    std::vector<uint32_t> m_oitIndices;

    VkBuffer m_oitVertexBuffer;
    VkDeviceMemory m_oitVertexBufferMemory;
    VkBuffer m_oitIndexBuffer;
    VkDeviceMemory m_oitIndexBufferMemory;

    std::array<VkCommandBuffer, m_framesInFlight> m_commandBuffers;
    std::array<VkCommandBuffer, m_framesInFlight> m_computeCommandBuffers;

    std::array<VkBuffer, m_framesInFlight> m_matrixBuffers;
    std::array<VkDeviceMemory, m_framesInFlight> m_matrixBuffersMemory;
    std::array<void*, m_framesInFlight> m_matrixBuffersMapped;

    std::array<VkBuffer, m_framesInFlight> m_oitCountBuffers;
    std::array<VkDeviceMemory, m_framesInFlight> m_oitCountBuffersMemory;
    std::array<void*, m_framesInFlight> m_oitCountBuffersMapped;

    static constexpr uint32_t s_listSize = 1900000;
    std::array<VkBuffer, m_framesInFlight> m_oitLinkLists;
    std::array<VkDeviceMemory, m_framesInFlight> m_oitLinkListsMemory;

    std::array<VkImage, m_framesInFlight> m_oitHeadImage;
    std::array<VkDeviceMemory, m_framesInFlight> m_oitHeadImageMemory;
    std::array<VkImageView, m_framesInFlight> m_oitHeadImageView;

    VkImage m_oitRenderImage;
    VkDeviceMemory m_oitRenderImageMemory;
    VkImageView m_oitRenderImageView;

    VkSampler m_blendTargetImageSampler;

    VkDescriptorPool m_descriptorPool;
    std::array<VkDescriptorSet, m_framesInFlight> m_opaqueDescriptorSet;
    std::array<VkDescriptorSet, m_framesInFlight> m_oitDescriptorSet;
    std::array<VkDescriptorSet, m_framesInFlight> m_blendDescriptorSet;
    
    std::array<VkFence, m_framesInFlight> m_computeFence;
    std::array<VkSemaphore, m_framesInFlight> m_oitFinishedSemaphores;

   
};