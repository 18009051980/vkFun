#pragma once

#include <Vertex.h>
#include <GlfwApp.h>

using MyVertex = Vertex<static_cast<VertexAttributeBits>(std::to_underlying(VertexAttributeBits::POSITION) | std::to_underlying(VertexAttributeBits::NORMAL))>;

class PBR : public GlfwApp
{
public:
    PBR() = default;
    ~PBR() = default;
    void init() override;
    void clean() override;
    void drawFrame() override;

    virtual void createFramebuffers() override;
    virtual void cleanupFramebuffers() override;

    virtual void cleanupDepthImage() override;
    virtual void createDepthImage() override;

    virtual void cleanupColorResources() override;
    virtual void createColorResources() override;

    void createDescriptorSetLayoutAndPool();
    void createPipelineLayout();

    void createRenderPass();

    void createPipeline();
    void createSMPipeline();
    
    void createUniformBuffer();
    void updateUniformBuffer();

    void createVertexBuffer();
    void createIndexBuffer();

    void recordCommandBufferAndSubmit(uint32_t imageIndex);

    void createSphere(glm::vec3 center, float radius, std::vector<MyVertex>& vertices, std::vector<uint32_t>& indices);

    void createCube(glm::vec3 center, float edge, std::vector<MyVertex>& vertices, std::vector<uint32_t>& indices);
    void createGround();

    void createSMSampler();
private:
    inline static constexpr uint32_t m_framesInFlight = 2;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorSetLayout m_smDescriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipelineLayout m_smPipelineLayout;

    VkDescriptorPool m_descriptorPool;
    std::array<VkDescriptorSet, m_framesInFlight> m_descriptorSets; 
    std::array<VkDescriptorSet, m_framesInFlight> m_smDescriptorSets; 
    VkPipeline m_pipeline;
    VkPipeline m_smPipeline;

    std::vector<VkFramebuffer> m_smFramebuffers;
    std::vector<VkFramebuffer> m_framebuffers;
    VkRenderPass m_smPass;
    VkRenderPass m_renderPass;

    VkImage m_depthImage;
    VkImageView m_depthImageView;
    VkDeviceMemory m_depthImageMemory;

    std::array<VkBuffer, m_framesInFlight> m_smMatrixBuffers;
    std::array<VkDeviceMemory, m_framesInFlight> m_smMatrixBuffersMemory;
    std::array<void*, m_framesInFlight> m_smMatrixBuffersMapped;

    std::array<VkBuffer, m_framesInFlight> m_matrixBuffers;
    std::array<VkDeviceMemory, m_framesInFlight> m_matrixBuffersMemory;
    std::array<void*, m_framesInFlight> m_matrixBuffersMapped;

    std::array<VkBuffer, m_framesInFlight> m_materialBuffers;
    std::array<VkDeviceMemory, m_framesInFlight> m_materialBuffersMemory;
    std::array<void*, m_framesInFlight> m_materialBuffersMapped;

    std::array<VkBuffer, m_framesInFlight> m_lightBuffers;
    std::array<VkDeviceMemory, m_framesInFlight> m_lightBuffersMemory;
    std::array<void*, m_framesInFlight> m_lightBuffersMapped;

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    std::vector<MyVertex> m_vertices;

    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    std::vector<uint32_t> m_indices;

    std::array<VkCommandBuffer, m_framesInFlight> m_commandBuffers;

    VkBuffer m_groundVertexBuffer;
    VkDeviceMemory m_groundVertexBufferMemory;
    std::vector<MyVertex> m_groundVertices;

    VkBuffer m_groundIndexBuffer;
    VkDeviceMemory m_groundIndexBufferMemory;
    std::vector<uint32_t> m_groundIndices;

    VkImage m_smImage;
    VkImageView m_smImageView;
    VkDeviceMemory m_smImageMemory;

    VkSampler m_smSampler;
};