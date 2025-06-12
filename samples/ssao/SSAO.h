#pragma once


#include <GlfwApp.h>

#include <ImageLoader.h>
#include <ObjLoader.h>


class SSAO : public GlfwApp
{
public:
    SSAO():GlfwApp()
    {
        
    }
    ~SSAO() override
    {
        
    }   

    void init() override;
    void clean() override;
    void drawFrame() override;

private:
    bool tick(float delta) override; 

private:
    void cleanupFramebuffers() override
    {
        for (int i = 0; i < m_framebuffers.size(); ++i)
        {
            vkDestroyFramebuffer(m_logicDevice, m_framebuffers[i], nullptr);
        }
    }    
    void createFramebuffers() override;

    void cleanupDepthImage() override;
    void createDepthImage() override;

    void createDescriptorSetLayout();
    void createPipelineLayout();

    void createRenderPass();

    void createCommandBuffer();

    void createDescriptorPool();

    void createTextureImage(const char* filename, VkImage &image, VkDeviceMemory& imageMemory, int& width, int& height);

    void loadModel(const char* filename);
    void createVertexBuffer(const std::vector<MyVertex> & vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);

    void createUniformBuffer();
    void allocateDescriptorSets();

    void updateUniformBuffer();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createGraphicsPipeline();

    void createColorResources() override;
    void cleanupColorResources() override;


private:
    std::vector<VkFramebuffer> m_framebuffers;
    VkRenderPass m_renderPass;
    
    VkPipeline m_pipeline;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorSetLayout m_ssaoDescriptorSetLayout;
    VkPipelineLayout m_pipelineLayout;
    VkPipelineLayout m_ssaoPipelineLayout;

    std::vector<VkCommandBuffer> m_commandBuffers;

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;

    std::vector<VkBuffer> m_matrixBuffers;
    std::vector<VkDeviceMemory> m_matrixBuffersMemory;
    std::vector<void *> m_matrixBuffersMapped;

    std::vector<VkBuffer> m_timeBuffers;
    std::vector<VkDeviceMemory> m_timeBuffersMemory;
    std::vector<void *> m_timeBuffersMapped;

    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkSampler m_textureSampler;

    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;

    VkImage m_vikingroomImage;
    VkDeviceMemory m_vikingroomImageMemory;
    VkImageView m_vikingroomImageView;
    int m_mipmapLevels;

    std::vector<MyVertex> m_vertices;
    std::vector<uint32_t> m_indices;

    VkImage m_positionImage;
    VkDeviceMemory m_positionImageMemory;
    VkImageView m_positionImageView;

    VkImage m_normalImage;
    VkDeviceMemory m_normalImageMemory;
    VkImageView m_normalImageView;

};
