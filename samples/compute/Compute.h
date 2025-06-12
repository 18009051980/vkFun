#pragma once


#include <Vertex.h>
#include <GlfwApp.h>

#include <vector>


using MyVertex = Vertex<static_cast<VertexAttributeBits>(std::to_underlying(VertexAttributeBits::POSITION) | std::to_underlying(VertexAttributeBits::COLOR) | std::to_underlying(VertexAttributeBits::NORMAL))>;

class Compute : public GlfwApp
{
public:
    Compute() = default;
    ~Compute() override = default;

    virtual void drawFrame() override;
    virtual void init() override;
    virtual void clean() override;

    virtual void createFramebuffers() override;
    virtual void cleanupFramebuffers() override;

    virtual void cleanupDepthImage() override;
    virtual void createDepthImage() override;

    virtual void cleanupColorResources() {}
    virtual void createColorResources() {}

private:
    void createRenderPass();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void allocateDescriptorSets();

    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();

    void createGraphicsPipelineLayout();
    void createGraphicsPipeline();

    void createComputePipelineLayout();
    void createComputePipeline();

    void updateUniformBuffer();

    void allocateCommandBuffer();
    void recordCommandBuffer(int imageIndex);

    void createShaderStorageBuffers();

    void createComputeDescriptorSetLayout();
    void allocateComputeDescriptorSet();

    void recordComputeCommandBuffer();

    void compute();
private:
    bool useGpu = true;

    inline static constexpr uint32_t s_particleCount = 655360;
    static constexpr uint32_t m_framesInFlight = 2;
    std::vector<VkFramebuffer> m_framebuffers;
    VkRenderPass m_renderPass;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;

    std::array<VkDescriptorSet, m_framesInFlight> m_graphicsDescriptorSet;


    VkPipelineLayout m_graphicsPipelineLayout;
    VkPipeline m_graphicsPipeline;

    VkPipelineLayout m_computePipelineLayout;
    VkPipeline m_computePipeline;

    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;

    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;

    std::array<VkBuffer, m_framesInFlight> m_uniformBuffer;
    std::array<VkDeviceMemory, m_framesInFlight> m_uniformBufferMemory;
    std::array<void*, m_framesInFlight> m_uniformBufferData;

    std::array<VkCommandBuffer, m_framesInFlight> m_graphicsCommand;
    std::array<VkCommandBuffer, m_framesInFlight> m_computeCommand;
    
    std::vector<VkBuffer> m_shaderStorageBuffers;
    std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;
    VkBuffer m_shaderStorageStagingBuffer;
    VkDeviceMemory m_shaderStorageStagingBufferMemory;
    void* m_shaderStorageBufferMappedData;
    
    VkDescriptorSetLayout m_computeDescriptorSetLayout;
    std::array<VkDescriptorSet, m_framesInFlight> m_computeDescriptorSet;

    std::array<VkSemaphore, m_framesInFlight> m_computeSemaphores;
    std::array<VkFence, m_framesInFlight> m_computeFence;

};