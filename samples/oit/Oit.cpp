#include "Oit.h"

#include <glm/gtc/matrix_transform.hpp>


struct UBO
{
    glm::mat4 mvp;
    glm::ivec4 viewport;
};
struct Node
{
    uint32_t next;
    float depth;
    float color[4];
};

void Oit::init()
{
    allocateCommandBuffer();

    createDescriptorSetLayout();
    createPipelineLayout();

    createRenderPass();

    createOpaqueGraphichPipeline();
    createOitGraphicsPipeline();
    createBlendComputePipeline();

    ObjLoader objLoader;
    objLoader.load("assets/objs/viking_room.obj", m_vertices, m_indices);
    createVertexBuffer(m_vertexBuffer, m_vertexBufferMemory, m_vertices);
    createIndexBuffer(m_indexBuffer, m_indexBufferMemory, m_indices);

    createSamplerImage();
    createTextureSampler(m_textureSampler);
    createTextureSampler(m_blendTargetImageSampler);

    createUniformBuffer();

    allocateDescriptorSets();

    createDepthImage();
    createColorResources();
    createFramebuffers();

    updateDescriptorSets();

    m_oitVertices = std::vector{
        MyVertex{
            glm::vec3(0.f, 0.f, -0.5f),
            glm::vec3(1.f, 0.f, 0.f),
            glm::vec2(0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
        },
        MyVertex{
            glm::vec3(1.f, 0.f, -0.5f),
            glm::vec3(0.f, 1.f, 1.f),
            glm::vec2(0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
        },
        MyVertex{
            glm::vec3(1.f, 1.f, -0.5f),
            glm::vec3(1.f, 0.f, 1.f),
            glm::vec2(0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
        },
        MyVertex{
            glm::vec3(0.f, 1.f, -0.5f),
            glm::vec3(0.f, 1.f, 0.f),
            glm::vec2(0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
        },
        MyVertex{
            glm::vec3(0.f, 0.f, 1.f),
            glm::vec3(1.f, 0.f, 0.f),
            glm::vec2(0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
        },
        MyVertex{
            glm::vec3(1.f, 0.f, 1.f),
            glm::vec3(0.f, 1.f, 1.f),
            glm::vec2(0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
        },
        MyVertex{
            glm::vec3(1.f, 1.f, 1.f),
            glm::vec3(1.f, 0.f, 1.f),
            glm::vec2(0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
        },
        MyVertex{
            glm::vec3(0.f, 1.f, 1.f),
            glm::vec3(0.f, 1.f, 0.f),
            glm::vec2(0.f, 0.f),
            glm::vec3(0.f, 0.f, 0.f),
        },
    };
    createVertexBuffer(m_oitVertexBuffer, m_oitVertexBufferMemory, m_oitVertices);
    m_oitIndices = std::vector<uint32_t>{
        0, 1, 2,
        0, 2, 3,
        4, 5, 6,
        4, 6, 7,
        0, 1, 5,
        0, 5, 4,
        1, 2, 6, 
        1, 6, 5,
        2, 3, 7,
        2, 7, 6,
        3, 0, 4,
        3, 4, 7,
    };
    createIndexBuffer(m_oitIndexBuffer, m_oitIndexBufferMemory, m_oitIndices);

    VkSemaphoreCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkFenceCreateInfo fenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        vkCreateSemaphore(m_logicDevice, &info, nullptr, &m_oitFinishedSemaphores[i]); 
        vkCreateFence(m_logicDevice, &fenceCreateInfo, nullptr, &m_computeFence[i]);
    }
 
}
void Oit::clean()
{
    vkDeviceWaitIdle(m_logicDevice);

    vkDestroyBuffer(m_logicDevice, m_vertexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_vertexBufferMemory, nullptr);
    vkDestroyBuffer(m_logicDevice, m_indexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_indexBufferMemory, nullptr);

    vkDestroyBuffer(m_logicDevice, m_oitVertexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_oitVertexBufferMemory, nullptr);
    vkDestroyBuffer(m_logicDevice, m_oitIndexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_oitIndexBufferMemory, nullptr);


    vkDestroyPipeline(m_logicDevice, m_opaquePipeline, nullptr);
    vkDestroyPipeline(m_logicDevice, m_oitPipeline, nullptr);
    vkDestroyPipeline(m_logicDevice, m_blendPipeline, nullptr);
    
    vkDestroyDescriptorPool(m_logicDevice, m_descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(m_logicDevice, m_opaqueDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_logicDevice, m_oitDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_logicDevice, m_blendDescriptorSetLayout, nullptr);

    vkDestroyPipelineLayout(m_logicDevice, m_opaquePipelineLayout, nullptr);
    vkDestroyPipelineLayout(m_logicDevice, m_oitPipelineLayout, nullptr);
    vkDestroyPipelineLayout(m_logicDevice, m_blendPipelineLayout, nullptr);


    vkDestroyRenderPass(m_logicDevice, m_renderPass, nullptr);


    vkDestroyImageView(m_logicDevice, m_vikingroomImageView, nullptr);
    vkDestroyImage(m_logicDevice, m_vikingroomImage, nullptr);
    vkFreeMemory(m_logicDevice, m_vikingroomImageMemory, nullptr);


    vkDestroySampler(m_logicDevice, m_textureSampler, nullptr);
    vkDestroySampler(m_logicDevice, m_blendTargetImageSampler, nullptr);
    
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        vkUnmapMemory(m_logicDevice, m_matrixBuffersMemory[i]);
        vkDestroyBuffer(m_logicDevice, m_matrixBuffers[i], nullptr);
        vkFreeMemory(m_logicDevice, m_matrixBuffersMemory[i], nullptr);

        vkUnmapMemory(m_logicDevice, m_oitCountBuffersMemory[i]);
        vkDestroyBuffer(m_logicDevice, m_oitCountBuffers[i], nullptr);
        vkFreeMemory(m_logicDevice, m_oitCountBuffersMemory[i], nullptr);

        vkDestroyBuffer(m_logicDevice, m_oitLinkLists[i], nullptr);
        vkFreeMemory(m_logicDevice, m_oitLinkListsMemory[i], nullptr);

        vkDestroySemaphore(m_logicDevice, m_oitFinishedSemaphores[i], nullptr); 
        vkDestroyFence(m_logicDevice, m_computeFence[i], nullptr); 
    }
    

}

void Oit::drawFrame()
{
    vkWaitForFences(m_logicDevice, 1, &m_inFlightFence[m_curFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_logicDevice, 1, &m_inFlightFence[m_curFrame]);

    uint32_t imageIndex{};
    auto ret = vkAcquireNextImageKHR(m_logicDevice, m_swapchain, UINT64_MAX, m_imageAvailabelesemaphores[m_curFrame], VK_NULL_HANDLE, &imageIndex);

    if (VK_ERROR_OUT_OF_DATE_KHR == ret)
    {
        recreateSwapchain();
        return;
    }

    vkResetCommandBuffer(m_commandBuffers[m_curFrame], 0);
    recordCommandBuffer(m_commandBuffers[m_curFrame], imageIndex);

    VkSubmitInfo renderSubmitInfo{};
    renderSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailabelesemaphores[m_curFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    renderSubmitInfo.waitSemaphoreCount = 1;
    renderSubmitInfo.pWaitSemaphores = waitSemaphores;
    renderSubmitInfo.pWaitDstStageMask = waitStages;
    renderSubmitInfo.commandBufferCount = 1;
    renderSubmitInfo.pCommandBuffers = &m_commandBuffers[m_curFrame];

    VkSemaphore signalSemaphores[] = {m_oitFinishedSemaphores[m_curFrame]};
    renderSubmitInfo.signalSemaphoreCount = 1;
    renderSubmitInfo.pSignalSemaphores = signalSemaphores;

    if (auto ret = vkQueueSubmit(m_graphicsAndComputeQueue, 1, &renderSubmitInfo, m_inFlightFence[m_curFrame]); ret != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    recordComputeCmdBuffer(imageIndex);
    vkDeviceWaitIdle(m_logicDevice);


    VkSemaphore prensetSemaphores[] = {m_renderFinishedSemaphores[m_curFrame]};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = prensetSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    ret = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (VK_ERROR_OUT_OF_DATE_KHR == ret || VK_SUBOPTIMAL_KHR == ret)
    {
        recreateSwapchain();
    }
    int * t = (int*)m_oitCountBuffersMapped[m_curFrame];
    // std::cout << t[0] << std::endl;

    m_curFrame = (m_curFrame + 1) & 1;
}


void Oit::createFramebuffers()
{
    m_framebuffers.resize(m_imageViews.size());
    for (size_t i = 0; i < m_imageViews.size(); i++)
    {
        VkImageView attachments[]{
            m_oitRenderImageView,
            m_depthImageView,
        };
        VkFramebufferCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderPass,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = m_extent.width,
            .height = m_extent.height,
            .layers = 1,
        };

        vkCreateFramebuffer(m_logicDevice, &createInfo, nullptr, &m_framebuffers[i]);
    }
    
}
void Oit::cleanupFramebuffers()
{
    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(m_logicDevice, m_framebuffers[i], nullptr);
    }
    m_framebuffers.clear();
}
void Oit::cleanupDepthImage()
{
    vkDestroyImageView(m_logicDevice, m_depthImageView, nullptr);
    vkDestroyImage(m_logicDevice, m_depthImage, nullptr);
    vkFreeMemory(m_logicDevice, m_depthImageMemory, nullptr);
}
void Oit::createDepthImage()
{
    this->createImage(
        m_extent.width,
        m_extent.height,
        1, 
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_depthImage,
        m_depthImageMemory
    );

    m_depthImageView = this->createImageView(
        m_depthImage,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1
    );

    VkCommandBuffer commandBuffer;
    allocateCommandBuffer(commandBuffer);
    startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    transitionImageLayout(
        commandBuffer,
        m_depthImage,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        0,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        1
    );
    endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);
}
void Oit::cleanupColorResources()
{
    vkDestroyImageView(m_logicDevice, m_oitRenderImageView, nullptr);
    vkDestroyImage(m_logicDevice, m_oitRenderImage, nullptr);
    vkFreeMemory(m_logicDevice, m_oitRenderImageMemory, nullptr);
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        vkDestroyImageView(m_logicDevice, m_oitHeadImageView[i], nullptr);
        vkDestroyImage(m_logicDevice, m_oitHeadImage[i], nullptr);
        vkFreeMemory(m_logicDevice, m_oitHeadImageMemory[i], nullptr);
    }
}
void Oit::createColorResources()
{
    this->createImage(m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_oitRenderImage, m_oitRenderImageMemory);

    m_oitRenderImageView = this->createImageView(m_oitRenderImage, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    VkCommandBuffer commandBuffer;
    allocateCommandBuffer(commandBuffer);
    startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    transitionImageLayout(commandBuffer, m_oitRenderImage, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
    endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);

    
    allocateCommandBuffer(commandBuffer);
    startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    for (size_t i = 0; i < m_swapchainsImages.size(); i++)
    {
        transitionImageLayout(commandBuffer, m_swapchainsImages[i], m_format, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);
    }
    
    endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);

    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        
        this->createImage(m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R32_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_oitHeadImage[i], m_oitHeadImageMemory[i]);

        m_oitHeadImageView[i] = this->createImageView(m_oitHeadImage[i], VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

        VkCommandBuffer commandBuffer;
        allocateCommandBuffer(commandBuffer);
        startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        transitionImageLayout(commandBuffer, m_oitHeadImage[i], VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);
        endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);
    }
    
}

void Oit::createDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };

    VkDescriptorSetLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    vkCreateDescriptorSetLayout(m_logicDevice, &createInfo, nullptr, &m_oitDescriptorSetLayout);

    std::vector<VkDescriptorSetLayoutBinding> opaqueBindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        }
    };
    VkDescriptorSetLayoutCreateInfo opaqueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(opaqueBindings.size()),
        .pBindings = opaqueBindings.data(),
    };
    vkCreateDescriptorSetLayout(m_logicDevice, &opaqueCreateInfo, nullptr, &m_opaqueDescriptorSetLayout);

    std::vector blendBindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 4,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 5,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
    };
    VkDescriptorSetLayoutCreateInfo blendCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(blendBindings.size()),
        .pBindings = blendBindings.data(),
    };
    vkCreateDescriptorSetLayout(m_logicDevice, &blendCreateInfo, nullptr, &m_blendDescriptorSetLayout);
}

void Oit::createPipelineLayout()
{
    VkPipelineLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_oitDescriptorSetLayout,
    };
    
    vkCreatePipelineLayout(m_logicDevice, &createInfo, nullptr, &m_oitPipelineLayout);

    VkPipelineLayoutCreateInfo opaqueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_opaqueDescriptorSetLayout,
    };
    vkCreatePipelineLayout(m_logicDevice, &opaqueCreateInfo, nullptr, &m_opaquePipelineLayout);

    VkPipelineLayoutCreateInfo blendCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_blendDescriptorSetLayout,
    };
    vkCreatePipelineLayout(m_logicDevice, &blendCreateInfo, nullptr, &m_blendPipelineLayout);
}

void Oit::createRenderPass()
{
    std::vector<VkAttachmentDescription> attachments{
        VkAttachmentDescription{
            .format = m_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        VkAttachmentDescription{
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };
    VkAttachmentReference colorRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference depthRef{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference depthRef2{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    
    std::vector<VkSubpassDescription> subpasses{
        VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorRef,
            .pDepthStencilAttachment = &depthRef,
        },
        VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorRef,
            .pDepthStencilAttachment = &depthRef2,
        },
    };
    std::vector dependency{
        VkSubpassDependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        },

        VkSubpassDependency{
            .srcSubpass = 0,
            .dstSubpass = 1,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT , 
        },
    };
    VkRenderPassCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = static_cast<uint32_t>(subpasses.size()),
        .pSubpasses = subpasses.data(),
        .dependencyCount = static_cast<uint32_t>(dependency.size()),
        .pDependencies = dependency.data(),
    };
    vkCreateRenderPass(m_logicDevice, &info, nullptr, &m_renderPass);    
}

void Oit::createSamplerImage()
{
    int width, height, channels;
    auto pixels = ImageLoader::loadImage("assets/pics/viking_room.png", &width, &height, &channels);

    this->createImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vikingroomImage, m_vikingroomImageMemory);

    
    m_vikingroomImageView = this->createImageView(
        m_vikingroomImage,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize size = width * height * 4;
    this->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer, stagingBufferMemory);

    void * data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, pixels, size);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    ImageLoader::freeImage(pixels);


    VkCommandBuffer commandBuffer;
    this->allocateCommandBuffer(commandBuffer);
    this->startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    transitionImageLayout(
        commandBuffer, 
        m_vikingroomImage, 
        VK_FORMAT_R8G8B8A8_SRGB, 
        VK_IMAGE_ASPECT_COLOR_BIT, 
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1
    );
    copyBufferToImage(commandBuffer, stagingBuffer, m_vikingroomImage, width, height);

    
    transitionImageLayout(
        commandBuffer,
        m_vikingroomImage, 
        VK_FORMAT_R8G8B8A8_SRGB, 
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1
    );
    this->endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);
    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);
}

void Oit::createBlendComputePipeline()
{
    auto code = readFile("blend.comp.spv");
    auto module = createShaderModule(code);

    VkPipelineShaderStageCreateInfo stage{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = module,
        .pName = "main",
    };
    VkComputePipelineCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = stage,
        .layout = m_blendPipelineLayout,
        .basePipelineIndex = 0,
    };
    vkCreateComputePipelines(m_logicDevice, VK_NULL_HANDLE, 1, &info, nullptr, &m_blendPipeline);

    vkDestroyShaderModule(m_logicDevice, module, nullptr);
}
void Oit::createOpaqueGraphichPipeline()
{
    auto vertexShader = createShaderModule(readFile("opaque.vert.spv"));
    auto fragShader = createShaderModule(readFile("opaque.frag.spv"));

    std::vector stages{
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShader,
            .pName = "main",
        },
    };

    VkPipelineVertexInputStateCreateInfo vInput{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = MyVertex::getVertexInputBinding(),
        .vertexAttributeDescriptionCount = MyVertex::getVertexInputAttributeDescriptionCount(),
        .pVertexAttributeDescriptions = MyVertex::getVertexInputAttributeDescription(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemle{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineViewportStateCreateInfo viewportSt{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterSt{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.f,
    };
    VkPipelineColorBlendAttachmentState attState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo blendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attState,
    };
    VkPipelineMultisampleStateCreateInfo multiSample{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .minDepthBounds = 0.f,
        .maxDepthBounds = 1.f,
    };
    VkDynamicState ds[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicSt{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = ds,
    };
    VkGraphicsPipelineCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = (uint32_t)stages.size(),
        .pStages = stages.data(),
        .pVertexInputState = &vInput,
        .pInputAssemblyState = &inputAssemle,
        .pViewportState = &viewportSt,
        .pRasterizationState = &rasterSt,
        .pMultisampleState = &multiSample,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &blendState,
        .pDynamicState = &dynamicSt,
        .layout = m_opaquePipelineLayout,
        .renderPass = m_renderPass,
        .subpass = 0,
    };
    vkCreateGraphicsPipelines(m_logicDevice, VK_NULL_HANDLE, 1, &info, nullptr, &m_opaquePipeline);
    
    vkDestroyShaderModule(m_logicDevice, vertexShader, nullptr);
    vkDestroyShaderModule(m_logicDevice, fragShader, nullptr);
}

void Oit::createOitGraphicsPipeline()
{
    auto vertexShader = createShaderModule(readFile("oit.vert.spv"));
    auto fragShader = createShaderModule(readFile("oit.frag.spv"));

    std::vector stages{
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShader,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShader,
            .pName = "main",
        },
    };

    VkPipelineVertexInputStateCreateInfo vInput{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = MyVertex::getVertexInputBinding(),
        .vertexAttributeDescriptionCount = MyVertex::getVertexInputAttributeDescriptionCount(),
        .pVertexAttributeDescriptions = MyVertex::getVertexInputAttributeDescription(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemle{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineViewportStateCreateInfo viewportSt{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterSt{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.f,
    };
    VkPipelineColorBlendAttachmentState attState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    VkPipelineColorBlendStateCreateInfo blendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attState,
    };
    VkPipelineMultisampleStateCreateInfo multiSample{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .minDepthBounds = 0.f,
        .maxDepthBounds = 1.f,
    };
    VkDynamicState ds[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicSt{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = ds,
    };
    VkGraphicsPipelineCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = (uint32_t)stages.size(),
        .pStages = stages.data(),
        .pVertexInputState = &vInput,
        .pInputAssemblyState = &inputAssemle,
        .pViewportState = &viewportSt,
        .pRasterizationState = &rasterSt,
        .pMultisampleState = &multiSample,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &blendState,
        .pDynamicState = &dynamicSt,
        .layout = m_oitPipelineLayout,
        .renderPass = m_renderPass,
        .subpass = 1,
    };
    vkCreateGraphicsPipelines(m_logicDevice, VK_NULL_HANDLE, 1, &info, nullptr, &m_oitPipeline);
    
    vkDestroyShaderModule(m_logicDevice, vertexShader, nullptr);
    vkDestroyShaderModule(m_logicDevice, fragShader, nullptr);
}


void Oit::createVertexBuffer(VkBuffer& buffer, VkDeviceMemory& memory, const std::vector<MyVertex>& vertices) 
{
    //创建一个临时的缓冲区，用于存储顶点数据
    VkDeviceSize bufferSize = sizeof(MyVertex) * vertices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
    
    copyBuffer(stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);
}


void Oit::createIndexBuffer(VkBuffer& buffer, VkDeviceMemory&memory, const std::vector<uint32_t>& indices)
{
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);

    copyBuffer(stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);

    
}


void Oit::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_extent;

    std::vector<VkClearValue> clearValues{
        VkClearValue{
            .color = {{0.0f, 0.0f, 0.0f, 1.0f}},
        },
        VkClearValue{
            .depthStencil = {1.0f, 0},
        },
    };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    VkClearColorValue clearColor = { 
        .uint32 = {0xFFFFFFFF},
     }; 
    VkImageSubresourceRange range = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1,
    };
    uint32_t zero = 0;
    uint32_t invalid = 0xFFFFFFFF;
    vkCmdClearColorImage(commandBuffer, m_oitHeadImage[m_curFrame], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);
    
    VkImageMemoryBarrier headimagebarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, // 清除操作是 TRANSFER 写入
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,    // 后续着色器需要读取
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,                      // 与清除时保持一致
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,                      // 如果不改变布局
        .image = m_oitHeadImage[m_curFrame],
        .subresourceRange = range
    };
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,      // 清除操作的阶段
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // 后续着色器阶段
        0, 0, nullptr, 0, nullptr, 1, &headimagebarrier
    );
        
    vkCmdFillBuffer(commandBuffer, m_oitCountBuffers[m_curFrame], 0, sizeof(uint32_t), zero);
    vkCmdFillBuffer(commandBuffer, m_oitCountBuffers[m_curFrame], 4, sizeof(uint32_t), s_listSize);

    // 在后续命令之前插入 Buffer Memory Barrier
    VkBufferMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,  // vkCmdFillBuffer 是 TRANSFER
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,  // 后续可能是 OIT 着色器读写
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = m_oitCountBuffers[m_curFrame],
        .offset = 0,
        .size = sizeof(uint32_t),
    };
    
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,  // 发生在 TRANSFER 阶段
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // 后续在着色器阶段使用
        0,
        0, nullptr,
        1, &barrier,
        0, nullptr
    );
    
    vkCmdFillBuffer(commandBuffer, m_oitLinkLists[m_curFrame], 0, s_listSize * sizeof(Node), invalid);
    
    // 2. 插入 Pipeline Barrier
    VkBufferMemoryBarrier listbarrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,  // 填充操作是 TRANSFER 写入
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, // 后续计算/图形阶段会读写
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = m_oitLinkLists[m_curFrame],
        .offset = 0,
        .size = sizeof(Node) * s_listSize  // 同步整个 Buffer
    };
    
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,          // 填充操作阶段
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |   // 后续计算阶段
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,   // 或图形阶段
        0, 0, nullptr, 1, &listbarrier, 0, nullptr
    );

    VkDescriptorImageInfo headImageInfo{
        .sampler = VK_NULL_HANDLE,
        .imageView = m_oitHeadImageView[m_curFrame],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_oitDescriptorSet[m_curFrame],
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &headImageInfo,
    };
    vkUpdateDescriptorSets(m_logicDevice, 1, &write, 0, nullptr);
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaquePipeline);

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    
    
    updateUniformBuffer();

    VkViewport viewport{0.f, 0.f, (float)m_extent.width, (float)m_extent.height, 0.f, 1.f};

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, m_extent};

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaquePipelineLayout, 0, 1, &m_opaqueDescriptorSet[m_curFrame], 0, nullptr);
    VkBuffer vertexBuffers [] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    

    if(true)
    {
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_oitPipeline);

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_oitPipelineLayout, 0, 1, &m_oitDescriptorSet  [m_curFrame], 0, nullptr);

        
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_oitVertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, m_oitIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_oitIndices.size()), 1, 0, 0, 0);
        
    }
    
    vkCmdEndRenderPass(commandBuffer);
    
    VkMemoryBarrier memBarrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | 
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT  // 计算着色器读取
    };
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,     // 渲染管线写入阶段
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,          // 计算管线读取阶段
        0, 1, &memBarrier, 0, nullptr, 0, nullptr
    );
    if (auto ret = vkEndCommandBuffer(commandBuffer); ret != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to record command buffer!");
    }

}

void Oit::recordComputeCmdBuffer(uint32_t imageIndex)
{
    vkWaitForFences(m_logicDevice, 1, &m_computeFence[m_curFrame], VK_TRUE, UINT32_MAX);
    vkResetFences(m_logicDevice, 1, &m_computeFence[m_curFrame]);

    vkResetCommandBuffer(m_computeCommandBuffers[m_curFrame], 0);
    
    startCommandBuffer(m_computeCommandBuffers[m_curFrame], 0),
    
    transitionImageLayout(m_computeCommandBuffers[m_curFrame], m_oitRenderImage, m_format, VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT , VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

    transitionImageLayout(m_computeCommandBuffers[m_curFrame], m_depthImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT , VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1);

    transitionImageLayout(m_computeCommandBuffers[m_curFrame], m_swapchainsImages[imageIndex], m_format, 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_GENERAL, 1);

        VkClearColorValue clearColor = { 
            .float32 = {0.f, 0.f, 0.f, 1.f},
         }; 
        VkImageSubresourceRange range = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        };
        vkCmdClearColorImage(m_computeCommandBuffers[m_curFrame], m_swapchainsImages[imageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &range);
        
        VkImageMemoryBarrier headimagebarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, // 清除操作是 TRANSFER 写入
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,    // 后续着色器需要读取
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,                      // 与清除时保持一致
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,                      // 如果不改变布局
            .image = m_swapchainsImages[imageIndex],
            .subresourceRange = range
        };
        vkCmdPipelineBarrier(
            m_computeCommandBuffers[m_curFrame],
            VK_PIPELINE_STAGE_TRANSFER_BIT,      // 清除操作的阶段
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // 后续着色器阶段
            0, 0, nullptr, 0, nullptr, 1, &headimagebarrier
        );
    
    VkDescriptorImageInfo info{
        .sampler = VK_NULL_HANDLE,
        .imageView = m_imageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkDescriptorImageInfo oitRenderImageInfo{
        .sampler = m_blendTargetImageSampler,
        .imageView = m_oitRenderImageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo targetDepthImageInfo{
        .sampler = m_blendTargetImageSampler,
        .imageView = m_depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo headImageInfo{
        .sampler = VK_NULL_HANDLE,
        .imageView = m_oitHeadImageView[m_curFrame],
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    std::vector writes{
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_blendDescriptorSet[m_curFrame],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &oitRenderImageInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_blendDescriptorSet[m_curFrame],
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &info,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_blendDescriptorSet[m_curFrame],
            .dstBinding = 4,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &targetDepthImageInfo,
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_blendDescriptorSet[m_curFrame],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &headImageInfo,
        },
    };
    
    vkUpdateDescriptorSets(m_logicDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);


    vkCmdBindPipeline(m_computeCommandBuffers[m_curFrame], VK_PIPELINE_BIND_POINT_COMPUTE, m_blendPipeline);

    vkCmdBindDescriptorSets(m_computeCommandBuffers[m_curFrame], VK_PIPELINE_BIND_POINT_COMPUTE, m_blendPipelineLayout, 0, 1, &m_blendDescriptorSet[m_curFrame], 0, nullptr);

    vkCmdDispatch(m_computeCommandBuffers[m_curFrame], m_extent.width / 16, m_extent.height / 16, 1);
    



    transitionImageLayout(m_computeCommandBuffers[m_curFrame], m_swapchainsImages[imageIndex], m_format, VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_SHADER_WRITE_BIT, 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

    transitionImageLayout(m_computeCommandBuffers[m_curFrame], m_depthImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_ACCESS_SHADER_READ_BIT , 0, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

    transitionImageLayout(m_computeCommandBuffers[m_curFrame], m_oitRenderImage, m_format, VK_IMAGE_ASPECT_COLOR_BIT, VK_ACCESS_SHADER_READ_BIT , VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

    vkEndCommandBuffer(m_computeCommandBuffers[m_curFrame]);

    VkPipelineStageFlags stages[] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    VkSubmitInfo renderSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_oitFinishedSemaphores[m_curFrame],
        .pWaitDstStageMask = stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_computeCommandBuffers[m_curFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_renderFinishedSemaphores[m_curFrame],
    };
    vkQueueSubmit(m_graphicsAndComputeQueue, 1, &renderSubmitInfo, m_computeFence[m_curFrame]);


}

void Oit::allocateCommandBuffer()
{
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        allocateCommandBuffer(m_commandBuffers[i]);
        allocateCommandBuffer(m_computeCommandBuffers[i]);
    }
}

void Oit::allocateCommandBuffer(VkCommandBuffer &commandBuffer)
{
    VkCommandBufferAllocateInfo info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    vkAllocateCommandBuffers(m_logicDevice, &info, &commandBuffer);   
}

void Oit::createUniformBuffer()
{
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        this->createBuffer(sizeof(UBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_matrixBuffers[i], m_matrixBuffersMemory[i]);
        
        vkMapMemory(m_logicDevice, m_matrixBuffersMemory[i], 0, sizeof(UBO), 0, &m_matrixBuffersMapped[i]);
    }

    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        this->createBuffer(sizeof(uint32_t) * 2, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_oitCountBuffers[i], m_oitCountBuffersMemory[i]);
        vkMapMemory(m_logicDevice, m_oitCountBuffersMemory[i], 0, 8, 0, &m_oitCountBuffersMapped[i]);

        this->createBuffer(s_listSize * sizeof(Node), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT , m_oitLinkLists[i], m_oitLinkListsMemory[i]);
    }
  
}
void Oit::updateUniformBuffer()
{

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();    

    UBO ubo;
    glm::mat4 model = glm::rotate(glm::mat4(1.f), time * glm::radians(30.f), glm::vec3(0.f, 0.f, 1.f));
    glm::mat4 view = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), m_extent.width / (float)m_extent.height, 0.1f, 10.f);

    proj[1][1] *= -1;
    
    ubo.mvp = proj * view * model;
    ubo.viewport = glm::ivec4(0, 0, m_extent.width, m_extent.height);
    memcpy(m_matrixBuffersMapped[m_curFrame], &ubo.mvp, sizeof(UBO));
}

void Oit::allocateDescriptorSets()
{
    std::vector poolSizes{
        VkDescriptorPoolSize {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 3 * m_framesInFlight,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 3 * m_framesInFlight,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 3 * m_framesInFlight,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 3 * m_framesInFlight,
        },
    };

    VkDescriptorPoolCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 6,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };
    vkCreateDescriptorPool(m_logicDevice, &info, nullptr, &m_descriptorPool);

    std::vector layouts{
        m_opaqueDescriptorSetLayout,
        m_opaqueDescriptorSetLayout,
    };
    VkDescriptorSetAllocateInfo allocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    vkAllocateDescriptorSets(m_logicDevice, &allocateInfo, m_opaqueDescriptorSet.data());

    layouts = std::vector{
        m_oitDescriptorSetLayout,
        m_oitDescriptorSetLayout,
    };
    allocateInfo = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    vkAllocateDescriptorSets(m_logicDevice, &allocateInfo, m_oitDescriptorSet.data());

    layouts = std::vector{
        m_blendDescriptorSetLayout,
        m_blendDescriptorSetLayout,
    };
    allocateInfo = VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };
    vkAllocateDescriptorSets(m_logicDevice, &allocateInfo, m_blendDescriptorSet.data());

}

void Oit::updateDescriptorSets()
{
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        VkDescriptorBufferInfo matrixBufferInfo{
            .buffer = m_matrixBuffers[i],
            .offset = 0,
            .range = sizeof(UBO),
        };
        VkDescriptorImageInfo samplerImageInfo{
            .sampler = m_textureSampler,
            .imageView = m_vikingroomImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        std::vector writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_opaqueDescriptorSet[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &matrixBufferInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_opaqueDescriptorSet[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &samplerImageInfo,
            },
            
        };
    
        vkUpdateDescriptorSets(m_logicDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
    
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        VkDescriptorBufferInfo matrixBufferInfo{
            .buffer = m_matrixBuffers[i],
            .offset = 0,
            .range = sizeof(UBO),
        };
        VkDescriptorBufferInfo listBufferInfo{
            .buffer = m_oitLinkLists[i],
            .offset = 0,
            .range = sizeof(Node) * s_listSize,
        };
        
        VkDescriptorBufferInfo countBufferInfo{
            .buffer = m_oitCountBuffers[i],
            .offset = 0,
            .range = sizeof(uint32_t) * 2,
        };
        std::vector writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_oitDescriptorSet[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &matrixBufferInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_oitDescriptorSet[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &listBufferInfo,
            },
            
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_oitDescriptorSet[i],
                .dstBinding = 3,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &countBufferInfo,
            },
        };
    
        vkUpdateDescriptorSets(m_logicDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
    
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        VkDescriptorImageInfo headImageInfo{
            .sampler = VK_NULL_HANDLE,
            .imageView = m_oitHeadImageView[i],
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        };
        VkDescriptorBufferInfo linkListInfo{
            .buffer = m_oitLinkLists[i],
            .offset = 0,
            .range = s_listSize * sizeof(Node),
        };
        VkDescriptorBufferInfo viewportBufferInfo{
            .buffer = m_matrixBuffers[i],
            .offset = 0,
            .range = sizeof(UBO),
        };
        std::vector writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_blendDescriptorSet[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &headImageInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_blendDescriptorSet[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &linkListInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_blendDescriptorSet[i],
                .dstBinding = 5,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &viewportBufferInfo,
            },
        };
        vkUpdateDescriptorSets(m_logicDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
    
}