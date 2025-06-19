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
    void createSSAODescriptorSetLayout();
    void createPipelineLayout();
    void createSSAOPipelineLayout();

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
    inline static constexpr uint8_t m_framesInFlight = 2; 

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

    std::vector<VkBuffer> m_matrixBuffers{m_framesInFlight};
    std::vector<VkDeviceMemory> m_matrixBuffersMemory{m_framesInFlight};
    std::vector<void *> m_matrixBuffersMapped{m_framesInFlight};

    std::vector<VkBuffer> m_timeBuffers{m_framesInFlight};
    std::vector<VkDeviceMemory> m_timeBuffersMemory{m_framesInFlight};
    std::vector<void *> m_timeBuffersMapped{m_framesInFlight};

    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkSampler m_textureSampler;

    std::vector<VkImage> m_depthImage{m_framesInFlight};
    std::vector<VkDeviceMemory> m_depthImageMemory{m_framesInFlight};
    std::vector<VkImageView> m_depthImageView{m_framesInFlight};
    VkSampler m_depthSampler;           //depthImage cant be used as image_usage_storage_image
    
    VkImage m_vikingroomImage;
    VkDeviceMemory m_vikingroomImageMemory;
    VkImageView m_vikingroomImageView;
    int m_mipmapLevels;

    std::vector<MyVertex> m_vertices;
    std::vector<uint32_t> m_indices;

    std::array<VkImage, m_framesInFlight> m_positionImage;
    std::array<VkDeviceMemory, m_framesInFlight> m_positionImageMemory;
    std::array<VkImageView, m_framesInFlight> m_positionImageView;

    std::array<VkImage, m_framesInFlight> m_normalImage;
    std::array<VkDeviceMemory, m_framesInFlight> m_normalImageMemory;
    std::array<VkImageView, m_framesInFlight> m_normalImageView;

};
