#include "Compute.h"

#include <random>

struct UBO
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

void Compute::init()
{
    createRenderPass();
    createDescriptorSetLayout();
    createDescriptorPool();
    createGraphicsPipelineLayout();
    createGraphicsPipeline();

    createComputeDescriptorSetLayout();
    createComputePipelineLayout();
    createComputePipeline();

    createDepthImage();
    createColorResources();
    
    createFramebuffers();
    
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();
    createShaderStorageBuffers();
    

    allocateCommandBuffer();

    allocateDescriptorSets();
    allocateComputeDescriptorSet();
    

    VkSemaphoreCreateInfo semaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkFenceCreateInfo fenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        vkCreateSemaphore(m_logicDevice, &semaphoreCreateInfo, nullptr, &m_computeSemaphores[i]);
        vkCreateFence(m_logicDevice, &fenceCreateInfo, nullptr, &m_computeFence[i]);
    }
    


}

void Compute::clean()
{
    vkDeviceWaitIdle(m_logicDevice);

    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        vkDestroyFence(m_logicDevice, m_computeFence[i], nullptr);
        vkDestroySemaphore(m_logicDevice, m_computeSemaphores[i], nullptr);
    }
    
    vkUnmapMemory(m_logicDevice, m_shaderStorageStagingBufferMemory);
    vkDestroyBuffer(m_logicDevice, m_shaderStorageStagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_shaderStorageStagingBufferMemory, nullptr);

    vkFreeCommandBuffers(m_logicDevice, m_commandPool, m_framesInFlight, m_graphicsCommand.data());
    vkFreeCommandBuffers(m_logicDevice, m_commandPool, m_framesInFlight, m_computeCommand.data());
    
    vkDestroyDescriptorPool(m_logicDevice, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_logicDevice, m_descriptorSetLayout, nullptr);

    vkDestroyDescriptorSetLayout(m_logicDevice, m_computeDescriptorSetLayout, nullptr);

    vkDestroyPipeline(m_logicDevice, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_logicDevice, m_graphicsPipelineLayout, nullptr);

    vkDestroyPipeline(m_logicDevice, m_computePipeline, nullptr);
    vkDestroyPipelineLayout(m_logicDevice, m_computePipelineLayout, nullptr);
    
    vkDestroyRenderPass(m_logicDevice, m_renderPass, nullptr);

    vkDestroyBuffer(m_logicDevice, m_vertexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_vertexBufferMemory, nullptr);

    vkDestroyBuffer(m_logicDevice, m_indexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_indexBufferMemory, nullptr);

    for (size_t i = 0; i < 2; i++)
    {
        vkDestroyBuffer(m_logicDevice, m_shaderStorageBuffers[i], nullptr);
        vkFreeMemory(m_logicDevice, m_shaderStorageBuffersMemory[i], nullptr);
    }
    

    for (size_t i = 0; i < m_uniformBuffer.size(); i++)
    {
        vkUnmapMemory(m_logicDevice, m_uniformBufferMemory[i]);

        vkDestroyBuffer(m_logicDevice, m_uniformBuffer[i], nullptr);
        vkFreeMemory(m_logicDevice, m_uniformBufferMemory[i], nullptr);
    }
    
}   

void Compute::drawFrame()
{
    updateUniformBuffer();
    
    compute();
    
    vkWaitForFences(m_logicDevice, 1, &m_inFlightFence[m_curFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_logicDevice, 1, &m_inFlightFence[m_curFrame]);

    uint32_t imageIndex{};

    auto ret = vkAcquireNextImageKHR(m_logicDevice, m_swapchain, UINT64_MAX, m_imageAvailabelesemaphores[m_curFrame], VK_NULL_HANDLE, &imageIndex);

    if (VK_ERROR_OUT_OF_DATE_KHR == ret)
    {
        recreateSwapchain();
        return;
    }

    vkResetCommandBuffer(m_graphicsCommand[m_curFrame], 0);
    recordCommandBuffer(imageIndex);

    std::vector<VkPipelineStageFlags> waitStages;
    std::vector<VkSemaphore> waitSem;

    if (useGpu)
    {
        waitStages = std::vector<VkPipelineStageFlags>{
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
        waitSem = std::vector<VkSemaphore>{
            m_computeSemaphores[m_curFrame], 
            m_imageAvailabelesemaphores[m_curFrame],
        };
    }
    else
    {
        waitStages = std::vector<VkPipelineStageFlags>{
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
        waitSem = std::vector<VkSemaphore>{
            m_imageAvailabelesemaphores[m_curFrame],
        };
    }

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(waitSem.size()),
        .pWaitSemaphores = waitSem.data(),
        .pWaitDstStageMask = waitStages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &m_graphicsCommand[m_curFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_renderFinishedSemaphores[m_curFrame],
    };

    
    if (vkQueueSubmit(m_graphicsAndComputeQueue, 1, &submitInfo, m_inFlightFence[m_curFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderFinishedSemaphores[m_curFrame],
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &imageIndex,
    };

    ret = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (VK_ERROR_OUT_OF_DATE_KHR == ret || VK_SUBOPTIMAL_KHR == ret)
    {
        recreateSwapchain();  
    }
    m_curFrame = (m_curFrame + 1) & 1;
}

void Compute::createFramebuffers()
{
    m_framebuffers.resize(m_swapchainsImages.size());
    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        std::vector<VkImageView> attachments{
            m_imageViews[i],
            m_depthImageView,
        };
        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderPass,
            .attachmentCount = 2,
            .pAttachments = attachments.data(),
            .width = m_extent.width,
            .height = m_extent.height,
            .layers = 1,
        };
        if (VK_SUCCESS != vkCreateFramebuffer(m_logicDevice, &framebufferInfo, nullptr, &m_framebuffers[i]))
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }    
    
}


void Compute::cleanupFramebuffers()
{
    for (int i = 0; i < m_framebuffers.size(); ++i)
    {
        vkDestroyFramebuffer(m_logicDevice, m_framebuffers[i], nullptr);
    }
}


void Compute::createRenderPass()
{
    std::vector<VkAttachmentDescription> attachments{
        VkAttachmentDescription{
            .flags = 0,
            .format = m_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        VkAttachmentDescription{
            .flags = 0,
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        }
    };

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    std::vector<VkSubpassDescription> subpasses{
        VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
            .pDepthStencilAttachment = &depthAttachmentRef,
        },
    };

    std::vector<VkSubpassDependency> dependencies{
        VkSubpassDependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        }
    };
    

    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = subpasses.data(),
        .dependencyCount = 1,
        .pDependencies = dependencies.data(),
    };

    if (VK_SUCCESS != vkCreateRenderPass(m_logicDevice, &renderPassInfo, nullptr, &m_renderPass))
    {
        throw std::runtime_error("failed to create render pass!");
    }

}

void Compute::cleanupDepthImage()
{
    vkDestroyImageView(m_logicDevice, m_depthImageView, nullptr);
    vkDestroyImage(m_logicDevice, m_depthImage, nullptr);
    vkFreeMemory(m_logicDevice, m_depthImageMemory, nullptr);
}
void Compute::createDepthImage()
{
    createImage(m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);

    m_depthImageView = createImageView(m_depthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

}

void Compute::createVertexBuffer()
{
    std::vector<MyVertex> vertices{
        MyVertex{glm::vec3{-0.5f, -0.5f, 0.0f}, glm::vec3{0.0f, 0.f, 0.f}, glm::vec3(0.f)},
        MyVertex{glm::vec3{0.5f, -0.5f, 0.0f}, glm::vec3{1.0f, 0.0f, 0.f}, glm::vec3(0.f)},
        MyVertex{glm::vec3{0.5f, 0.5f, 0.0f}, glm::vec3{1.0f, 1.0f, 0.f}, glm::vec3(0.f)},
        MyVertex{glm::vec3{-0.5f, 0.5f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.f}, glm::vec3(0.f)},
    };

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VulkanEnv::createBuffer(sizeof(MyVertex) * vertices.size(),  VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT , stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, sizeof(MyVertex) * vertices.size(), 0, &data);
    memcpy(data, vertices.data(), sizeof(MyVertex) * vertices.size());
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    VulkanEnv::createBuffer(sizeof(MyVertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

    copyBuffer(stagingBuffer, m_vertexBuffer, sizeof(MyVertex) * vertices.size());

    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);

}

void Compute::createIndexBuffer()
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    std::vector<uint32_t> indices{
        0, 1, 2, 
        2, 3, 0,
    };

    VulkanEnv::createBuffer(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, sizeof(uint32_t) * indices.size(), 0, &data);

    memcpy(data, indices.data(), sizeof(uint32_t) * indices.size());
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    VulkanEnv::createBuffer(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);
    copyBuffer(stagingBuffer, m_indexBuffer, sizeof(uint32_t) * indices.size());

    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);
}

void Compute::createDescriptorSetLayout()
{
    std::vector bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        }
    };
    
    VkDescriptorSetLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = bindings.data(),
    };

    if (VK_SUCCESS != vkCreateDescriptorSetLayout(m_logicDevice, &createInfo, nullptr, &m_descriptorSetLayout))
    {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void Compute::createDescriptorPool()
{
    std::vector poolSizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 4,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 2 * m_framesInFlight,
        }
    };
    

    VkDescriptorPoolCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 4,
        .poolSizeCount = 2,
        .pPoolSizes = poolSizes.data(),
    };
    
    if (VK_SUCCESS != vkCreateDescriptorPool(m_logicDevice, &createInfo, nullptr, &m_descriptorPool))
    {
        throw std::runtime_error("failed to create descriptor pool");
    }
}

void Compute::createUniformBuffer()
{

    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        this->createBuffer(sizeof(UBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_uniformBuffer[i], m_uniformBufferMemory[i]);
        vkMapMemory(m_logicDevice, m_uniformBufferMemory[i], 0, sizeof(UBO), 0, &m_uniformBufferData[i]);
    }
    
}

void Compute::updateUniformBuffer()
{
    UBO ubo{
        .model = glm::mat4(1.f),
        .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), m_extent.width / (float) m_extent.height, 0.1f, 10.f),
    };

    ubo.proj[1][1] *= -1.f;

    memcpy(m_uniformBufferData[m_curFrame], &ubo, sizeof(UBO));
}

void Compute::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("vert.spv");
    auto fragShaderCode = readFile("frag.spv");
    auto vertShaderModule = createShaderModule(vertShaderCode);
    auto fragShaderModule = createShaderModule(fragShaderCode);

    std::vector shaderStages{
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main",
        }
    };
    VkPipelineVertexInputStateCreateInfo vertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = MyVertex::getVertexInputBinding(),
        .vertexAttributeDescriptionCount = MyVertex::getVertexInputAttributeDescriptionCount(),
        .pVertexAttributeDescriptions = MyVertex::getVertexInputAttributeDescription(),
    };
    
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    };

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    
    VkPipelineRasterizationStateCreateInfo rasterState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.f,
    };

    VkPipelineMultisampleStateCreateInfo multisampleState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo depthState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    std::vector dynamicStates{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates.data(),
    };

    VkGraphicsPipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = m_graphicsPipelineLayout,
        .renderPass = m_renderPass,
        .subpass = 0,
    };

    if (VK_SUCCESS != vkCreateGraphicsPipelines(m_logicDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_graphicsPipeline))
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(m_logicDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_logicDevice, fragShaderModule, nullptr);
}

void Compute::createGraphicsPipelineLayout()
{
    VkPipelineLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayout,
    };

    if (VK_SUCCESS != vkCreatePipelineLayout(m_logicDevice, &createInfo, nullptr, &m_graphicsPipelineLayout))
    {
        throw std::runtime_error("failed to create pipeline layout");
    }
}

void Compute::allocateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(2, m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = (uint32_t)layouts.size(),
        .pSetLayouts = layouts.data(),
    };

    vkAllocateDescriptorSets(m_logicDevice, &allocInfo, m_graphicsDescriptorSet.data());

    std::vector<VkDescriptorBufferInfo> bufferInfos{
        VkDescriptorBufferInfo{
            .buffer = m_uniformBuffer[0],
            .offset = 0,
            .range = sizeof(UBO),
        },
        VkDescriptorBufferInfo{
            .buffer = m_uniformBuffer[1],
            .offset = 0,
            .range = sizeof(UBO),
        },
    };

    std::vector<VkWriteDescriptorSet> descriptorWrites{
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_graphicsDescriptorSet[0],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfos[0],
            .pTexelBufferView = nullptr
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_graphicsDescriptorSet[1],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfos[1],
            .pTexelBufferView = nullptr
        },
    };

    vkUpdateDescriptorSets(m_logicDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

}

void Compute::allocateCommandBuffer()
{
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        VulkanEnv::allocateCommandBuffer(m_graphicsCommand[i], VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VulkanEnv::allocateCommandBuffer(m_computeCommand[i], VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        
    }
}

void Compute::recordCommandBuffer(int imageIndex)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    if (vkBeginCommandBuffer(m_graphicsCommand[m_curFrame], &commandBufferBeginInfo) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    std::vector<VkClearValue> clearValues{
        VkClearValue{
            .color = {{0.0f, 0.0f, 0.0f, 1.0f}},
        },
        VkClearValue{
            .depthStencil = {1.0f, 0},
        },
    };
    VkRenderPassBeginInfo renderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_renderPass,
        .framebuffer = m_framebuffers[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = m_extent,
        },
        .clearValueCount = 2,
        .pClearValues = clearValues.data(),
    };

    vkCmdBeginRenderPass(m_graphicsCommand[m_curFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_graphicsCommand[m_curFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    VkDeviceSize offsets = {0};
    vkCmdBindVertexBuffers(m_graphicsCommand[m_curFrame], 0, 1, &m_shaderStorageBuffers[m_curFrame], &offsets);

    // vkCmdBindIndexBuffer(m_graphicsCommand[m_curFrame], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    VkViewport viewport{0.f, 0.f, (float)m_extent.width, (float)m_extent.height, 0.f, 1.f};

    vkCmdSetViewport(m_graphicsCommand[m_curFrame], 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, m_extent};

    vkCmdSetScissor(m_graphicsCommand[m_curFrame], 0, 1, &scissor);

    vkCmdBindDescriptorSets(m_graphicsCommand[m_curFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout, 0, 1, &m_graphicsDescriptorSet[m_curFrame], 0, nullptr);

    // vkCmdDrawIndexed(m_graphicsCommand[m_curFrame], 6, 1, 0, 0, 0);
    vkCmdDraw(m_graphicsCommand[m_curFrame], s_particleCount, 1, 0, 0);
    vkCmdEndRenderPass(m_graphicsCommand[m_curFrame]);

    if (vkEndCommandBuffer(m_graphicsCommand[m_curFrame]) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}


void Compute::createComputePipelineLayout()
{
    VkPipelineLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_computeDescriptorSetLayout,
    };
    if (VK_SUCCESS != vkCreatePipelineLayout(m_logicDevice, &createInfo, nullptr, &m_computePipelineLayout))
    {
        throw std::runtime_error("failed to create compute pipeline Layout");
    }
}

void Compute::createComputePipeline()
{
    auto code = readFile("comp.spv");
    auto shaderModule = createShaderModule(code);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderModule,
        .pName = "main",
    };

    VkComputePipelineCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = computeShaderStageInfo,
        .layout = m_computePipelineLayout,
    };

    if (vkCreateComputePipelines(m_logicDevice, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_computePipeline))
    {
        throw std::runtime_error("failed to create compute pipeline");
    }

    vkDestroyShaderModule(m_logicDevice, shaderModule, nullptr);
}

void Compute::createShaderStorageBuffers()
{
    m_shaderStorageBuffers.resize(m_framesInFlight);
    m_shaderStorageBuffersMemory.resize(m_framesInFlight);

    std::default_random_engine rndEngine{static_cast<uint32_t>(time(nullptr))};
    std::uniform_real_distribution<float> rndDist(0.f, 1.f);

    std::vector<MyVertex> particles(s_particleCount);
    for (auto& par : particles)
    {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2 * 3.1416f;

        float x = r * cos(theta);
        float y = r * sin(theta);
        float z = 0.f;

        par.position = glm::vec3(x,y,z);
        par.color = glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine));
        par.normal = glm::normalize(par.position) * 0.0002f;
        // par.normal = glm::normalize(glm::vec3(0.f,1.f,0.f)) * 0.0002f;
    }

    VkDeviceSize size = sizeof(MyVertex) * particles.size();

    this->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_shaderStorageStagingBuffer, m_shaderStorageStagingBufferMemory);

    vkMapMemory(m_logicDevice, m_shaderStorageStagingBufferMemory, 0, size, 0, &m_shaderStorageBufferMappedData);
    memcpy(m_shaderStorageBufferMappedData, particles.data(), size);
    

    for (size_t i = 0; i < m_shaderStorageBuffers.size(); i++)
    {
        this->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_shaderStorageBuffers[i], m_shaderStorageBuffersMemory[i]);

        this->copyBuffer(m_shaderStorageStagingBuffer, m_shaderStorageBuffers[i], size);
    }
}

void Compute::createComputeDescriptorSetLayout()
{
    std::vector bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
        },
    };

    VkDescriptorSetLayoutCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    if (VK_SUCCESS != vkCreateDescriptorSetLayout(m_logicDevice, &createInfo, nullptr, &m_computeDescriptorSetLayout))
    {
        throw std::runtime_error("failed to create compute descriptor set layout");
    }
}

void Compute::allocateComputeDescriptorSet()
{
    std::vector descriptorSetLayouts(2, m_computeDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = 2,
        .pSetLayouts = descriptorSetLayouts.data(),
    };

    vkAllocateDescriptorSets(m_logicDevice, &allocInfo, m_computeDescriptorSet.data());

    for (size_t i = 0; i < m_computeDescriptorSet.size(); i++)
    {
        VkDescriptorBufferInfo timeBufferInfo{
            .buffer = m_uniformBuffer[i],
            .offset = 0,
            .range = sizeof(UBO),
        };

        VkDescriptorBufferInfo lastFrame{
            .buffer = m_shaderStorageBuffers[(i - 1) % m_framesInFlight],
            .offset = 0,
            .range = sizeof(MyVertex) * s_particleCount,
        };
        VkDescriptorBufferInfo curFrame{
            .buffer = m_shaderStorageBuffers[i],
            .offset = 0,
            .range = sizeof(MyVertex) * s_particleCount,
        };

        std::vector writes{
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_computeDescriptorSet[i],
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &timeBufferInfo
            },
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_computeDescriptorSet[i],
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &lastFrame
            },
            VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_computeDescriptorSet[i],
                .dstBinding = 2,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &curFrame
            },
        };

        vkUpdateDescriptorSets(m_logicDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        
    }
    
}

void Compute::recordComputeCommandBuffer()
{
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    vkBeginCommandBuffer(m_computeCommand[m_curFrame], &beginInfo);

    vkCmdBindPipeline(m_computeCommand[m_curFrame], VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

    vkCmdBindDescriptorSets(m_computeCommand[m_curFrame], VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSet[m_curFrame], 0, nullptr);

    vkCmdDispatch(m_computeCommand[m_curFrame], s_particleCount / 256, 1, 1);

    vkEndCommandBuffer(m_computeCommand[m_curFrame]);
}

void Compute::compute()
{

    if (useGpu)
    {
        vkWaitForFences(m_logicDevice, 1, &m_computeFence[m_curFrame], VK_TRUE, UINT32_MAX);
        vkResetFences(m_logicDevice, 1, &m_computeFence[m_curFrame]);

        vkResetCommandBuffer(m_computeCommand[m_curFrame], 0);
        recordComputeCommandBuffer();


        VkSubmitInfo computeSubmit{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &m_computeCommand[m_curFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &m_computeSemaphores[m_curFrame],
        };
        if (vkQueueSubmit(m_graphicsAndComputeQueue, 1, &computeSubmit, m_computeFence[m_curFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit compute command buffer!");
        }
    }
    else
    {
        MyVertex* vertices = static_cast<MyVertex*>(m_shaderStorageBufferMappedData);
        if (!vertices)
        {
            return;
        }
        auto com = [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; i++)
            {
                vertices[i].position += vertices[i].normal;

                if (vertices[i].position.x >= 1.f || vertices[i].position.x <= -1.f)
                {
                    vertices[i].normal *= -1;
                }
                if (vertices[i].position.y >= 1.f || vertices[i].position.y <= -1.f)
                {
                    vertices[i].normal *= -1;
                }
            }   
        };
        constexpr int threadNum = 8;
        std::thread t[threadNum];

        for (size_t i = 0; i < threadNum; i++)
        {
            size_t start = static_cast<size_t>(float(i) / threadNum * s_particleCount);
            size_t end = static_cast<size_t>(float(i + 1) / threadNum * s_particleCount);
            t[i] = std::thread(com, start, end);
        }
        
        for (size_t i = 0; i < threadNum; i++)
        {
            t[i].join();
        }
        
        this->copyBuffer(m_shaderStorageStagingBuffer, m_shaderStorageBuffers[m_curFrame], s_particleCount * sizeof(MyVertex));
    }

}