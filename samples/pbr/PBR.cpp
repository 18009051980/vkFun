#include "PBR.h"


struct Mvp
{
    glm::mat4 mvp;
};
struct Matrix
{
    glm::vec4 viewPos;
    glm::mat4 model;
    glm::mat4 mvp;
    glm::mat4 lightMvp;
};

struct Material
{
    glm::vec3 albedo;
    float metallic;
    float roughness;
};
struct Light
{
    glm::vec3 position;
    float intensity;
    glm::vec3 color;
};

void PBR::init()
{
    createUniformBuffer();
    createDepthImage();

    createSMSampler();

    createDescriptorSetLayoutAndPool();
    createPipelineLayout();

    createRenderPass();

    createFramebuffers();

    createSMPipeline();
    createPipeline();

    createSphere(glm::vec3(0), 1, m_vertices, m_indices);

    createVertexBuffer();
    createIndexBuffer();

    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        this->allocateCommandBuffer(m_commandBuffers[i], VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }
    createGround();
    

}
void PBR::clean()
{
    vkDeviceWaitIdle(m_logicDevice);

    vkDestroySampler(m_logicDevice, m_smSampler, nullptr);
    
   
    vkDestroyPipeline(m_logicDevice, m_pipeline, nullptr);
    vkDestroyPipeline(m_logicDevice, m_smPipeline, nullptr);
    vkDestroyRenderPass(m_logicDevice, m_renderPass, nullptr);
    vkDestroyRenderPass(m_logicDevice, m_smPass, nullptr);

    vkDestroyPipelineLayout(m_logicDevice, m_smPipelineLayout, nullptr);
    vkDestroyPipelineLayout(m_logicDevice, m_pipelineLayout, nullptr);

    vkDestroyDescriptorPool(m_logicDevice, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_logicDevice, m_descriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_logicDevice, m_smDescriptorSetLayout, nullptr);

    for (size_t i = 0; i < m_framesInFlight; i++)
    {   
        vkUnmapMemory(m_logicDevice, m_matrixBuffersMemory[i]);
        vkDestroyBuffer(m_logicDevice, m_matrixBuffers[i], nullptr);
        vkFreeMemory(m_logicDevice, m_matrixBuffersMemory[i], nullptr);

        vkUnmapMemory(m_logicDevice, m_materialBuffersMemory[i]);
        vkDestroyBuffer(m_logicDevice, m_materialBuffers[i], nullptr);
        vkFreeMemory(m_logicDevice, m_materialBuffersMemory[i], nullptr);

        vkUnmapMemory(m_logicDevice, m_lightBuffersMemory[i]);
        vkDestroyBuffer(m_logicDevice, m_lightBuffers[i], nullptr);
        vkFreeMemory(m_logicDevice, m_lightBuffersMemory[i], nullptr);

        vkUnmapMemory(m_logicDevice, m_smMatrixBuffersMemory[i]);
        vkDestroyBuffer(m_logicDevice, m_smMatrixBuffers[i], nullptr);
        vkFreeMemory(m_logicDevice, m_smMatrixBuffersMemory[i], nullptr);
    }
    vkDestroyBuffer(m_logicDevice, m_vertexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_vertexBufferMemory, nullptr);

    vkDestroyBuffer(m_logicDevice, m_indexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_indexBufferMemory, nullptr);

    vkDestroyBuffer(m_logicDevice, m_groundVertexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_groundVertexBufferMemory, nullptr);

    vkDestroyBuffer(m_logicDevice, m_groundIndexBuffer, nullptr);
    vkFreeMemory(m_logicDevice, m_groundIndexBufferMemory, nullptr);
    
}

void PBR::drawFrame()
{
    vkWaitForFences(m_logicDevice, 1, &m_inFlightFence[m_curFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_logicDevice, 1, &m_inFlightFence[m_curFrame]);

    uint32_t imageIndex;
    if (auto ret = vkAcquireNextImageKHR(m_logicDevice, m_swapchain, UINT64_MAX, m_imageAvailabelesemaphores[m_curFrame], VK_NULL_HANDLE, &imageIndex); ret == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }

    recordCommandBufferAndSubmit(imageIndex);

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderFinishedSemaphores[m_curFrame],
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &imageIndex,
    };

    auto ret = vkQueuePresentKHR(m_graphicsAndComputeQueue, &presentInfo);
    if(ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapchain();
    }
    m_curFrame = (m_curFrame + 1) % m_framesInFlight;
}

void PBR::createFramebuffers() 
{
    m_framebuffers.resize(m_imageViews.size());
    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        VkImageView attachements[] = {m_imageViews[i], m_depthImageView};
        
        VkFramebufferCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_renderPass,
            .attachmentCount = 2,
            .pAttachments = attachements,
            .width = m_extent.width,
            .height = m_extent.height,
            .layers = 1,
        };
        vkCreateFramebuffer(m_logicDevice, &info, nullptr, &m_framebuffers[i]);
    }
    m_smFramebuffers.resize(m_imageViews.size());
    for (size_t i = 0; i < m_smFramebuffers.size(); i++)
    {
        VkImageView attachements[] = {m_smImageView};
        
        VkFramebufferCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_smPass,
            .attachmentCount = 1,
            .pAttachments = attachements,
            .width = m_extent.width,
            .height = m_extent.height,
            .layers = 1,
        };
        vkCreateFramebuffer(m_logicDevice, &info, nullptr, &m_smFramebuffers[i]);
    }
}
void PBR::cleanupFramebuffers() 
{
    for (size_t i = 0; i < m_framebuffers.size(); i++)
    {
        vkDestroyFramebuffer(m_logicDevice, m_framebuffers[i], nullptr);
        vkDestroyFramebuffer(m_logicDevice, m_smFramebuffers[i], nullptr);

    }
    
}
void PBR::cleanupDepthImage() 
{
    vkDestroyImageView(m_logicDevice, m_depthImageView, nullptr);
    vkDestroyImage(m_logicDevice, m_depthImage, nullptr);
    vkFreeMemory(m_logicDevice, m_depthImageMemory, nullptr);

    vkDestroyImageView(m_logicDevice, m_smImageView, nullptr);
    vkDestroyImage(m_logicDevice, m_smImage, nullptr);
    vkFreeMemory(m_logicDevice, m_smImageMemory, nullptr);
}
void PBR::createDepthImage() 
{
    this->createImage(m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
    m_depthImageView = this->createImageView(m_depthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    this->createImage(m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_smImage, m_smImageMemory);
    m_smImageView = this->createImageView(m_smImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    
}
void PBR::cleanupColorResources() 
{

}
void PBR::createColorResources() 
{

}

void PBR::createDescriptorSetLayoutAndPool()
{
    std::vector bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    VkDescriptorSetLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    vkCreateDescriptorSetLayout(m_logicDevice, &info, nullptr, &m_descriptorSetLayout);

    std::vector smBindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        },
    };

    info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(smBindings.size()),
        .pBindings = smBindings.data(),
    };

    vkCreateDescriptorSetLayout(m_logicDevice, &info, nullptr, &m_smDescriptorSetLayout);

    std::vector size{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(bindings.size() + smBindings.size()) * m_framesInFlight,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 2,
        }
    };

    VkDescriptorPoolCreateInfo poolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 4,
        .poolSizeCount = 2,
        .pPoolSizes = size.data(),
    };

    vkCreateDescriptorPool(m_logicDevice, &poolCreateInfo, nullptr, &m_descriptorPool);
    
    VkDescriptorSetLayout layouts[] = {m_descriptorSetLayout, m_descriptorSetLayout};

    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = m_framesInFlight,
        .pSetLayouts = layouts,
    };

    vkAllocateDescriptorSets(m_logicDevice, &allocInfo, m_descriptorSets.data());

    VkDescriptorSetLayout smLayouts[] = {m_smDescriptorSetLayout, m_smDescriptorSetLayout};

    allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = m_framesInFlight,
        .pSetLayouts = smLayouts,
    };

    vkAllocateDescriptorSets(m_logicDevice, &allocInfo, m_smDescriptorSets.data());

    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        VkDescriptorBufferInfo matrixBufferInfo{
            .buffer = m_matrixBuffers[i],
            .offset = 0,
            .range = sizeof(Matrix),
        };
        VkDescriptorBufferInfo materialBufferInfo{
            .buffer = m_materialBuffers[i],
            .offset = 0,
            .range = sizeof(Material),
        };
        VkDescriptorBufferInfo lightBufferInfo{
            .buffer = m_lightBuffers[i],
            .offset = 0,
            .range = sizeof(Light),
        };
        VkDescriptorBufferInfo smMatrixInfo{
            .buffer = m_smMatrixBuffers[i],
            .offset = 0,
            .range = sizeof(Mvp),
        };
        std::vector<VkWriteDescriptorSet> writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &matrixBufferInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &materialBufferInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &lightBufferInfo,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_smDescriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &smMatrixInfo,
            }
        };

        vkUpdateDescriptorSets(m_logicDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    }
    
}

void PBR::createPipelineLayout()
{
    std::vector layouts{
        m_descriptorSetLayout,
    };
    VkPipelineLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = layouts.data(),
    };
    vkCreatePipelineLayout(m_logicDevice, &info, nullptr, &m_pipelineLayout);

    layouts = {
        m_smDescriptorSetLayout,
    };
    info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = layouts.data(),
    };
    vkCreatePipelineLayout(m_logicDevice, &info, nullptr, &m_smPipelineLayout);
}

void PBR::createRenderPass()
{
    std::vector attachments{
        VkAttachmentDescription{
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
    };

    VkAttachmentReference smRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    std::vector subpass{
        VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .pDepthStencilAttachment = &smRef,
        },
    };

    std::vector dependency{
        VkSubpassDependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        },
        VkSubpassDependency{
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            },
    };

    VkRenderPassCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = subpass.data(),
        .dependencyCount = 2,
        .pDependencies = dependency.data(),
    };

    vkCreateRenderPass(m_logicDevice, &info, nullptr, &m_smPass);
    attachments = {
        VkAttachmentDescription{
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
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
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
    
    subpass = {
        VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorRef,
            .pDepthStencilAttachment = &depthRef,
        },
    };

    dependency = {
        VkSubpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
    },
        VkSubpassDependency{
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT ,
        },
    };

    info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = subpass.data(),
        .dependencyCount = 2,
        .pDependencies = dependency.data(),
    };

    vkCreateRenderPass(m_logicDevice, &info, nullptr, &m_renderPass);
}

void PBR::createPipeline()
{
    auto vertCode = this->readFile("pbr.vert.spv");
    auto fragCode = this->readFile("pbr.frag.spv");
    auto vertexShader = this->createShaderModule(vertCode);
    auto fragShader = this->createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderInfo []{
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
    
    VkPipelineVertexInputStateCreateInfo vertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = MyVertex::getVertexInputBinding(),
        .vertexAttributeDescriptionCount = MyVertex::getVertexInputAttributeDescriptionCount(),
        .pVertexAttributeDescriptions = MyVertex::getVertexInputAttributeDescription(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_TRUE,
    };


    VkPipelineViewportStateCreateInfo viewportInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.f,
    };
    VkPipelineMultisampleStateCreateInfo multiSampleInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo depthStateInfo{
        .sType =VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable =VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState attachments[] = {
        VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, 
        },
    };
    VkPipelineColorBlendStateCreateInfo blendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = attachments,
    };
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

    VkGraphicsPipelineCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderInfo,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &rasterInfo,
        .pMultisampleState = &multiSampleInfo,
        .pDepthStencilState = &depthStateInfo,
        .pColorBlendState = &blendState,
        .pDynamicState = &dynamicInfo,
        .layout = m_pipelineLayout,
        .renderPass = m_renderPass,
        .subpass = 0,

    };
    vkCreateGraphicsPipelines(m_logicDevice, VK_NULL_HANDLE, 1, &info, nullptr, &m_pipeline);

    vkDestroyShaderModule(m_logicDevice, vertexShader, nullptr);
    vkDestroyShaderModule(m_logicDevice, fragShader, nullptr);
}



void PBR::createUniformBuffer()
{
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        this->createBuffer(sizeof(Matrix), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_matrixBuffers[i], m_matrixBuffersMemory[i]);
        vkMapMemory(m_logicDevice, m_matrixBuffersMemory[i], 0, sizeof(Matrix), 0, &m_matrixBuffersMapped[i]);

        this->createBuffer(sizeof(Material), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_materialBuffers[i], m_materialBuffersMemory[i]);
        vkMapMemory(m_logicDevice, m_materialBuffersMemory[i], 0, sizeof(Material), 0, &m_materialBuffersMapped[i]);

        this->createBuffer(sizeof(Light), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_lightBuffers[i], m_lightBuffersMemory[i]);
        vkMapMemory(m_logicDevice, m_lightBuffersMemory[i], 0, sizeof(Light), 0, &m_lightBuffersMapped[i]);

        this->createBuffer(sizeof(Mvp), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_smMatrixBuffers[i], m_smMatrixBuffersMemory[i]);
        vkMapMemory(m_logicDevice, m_smMatrixBuffersMemory[i], 0, sizeof(Matrix), 0, &m_smMatrixBuffersMapped[i]);
    }
    
}
void PBR::updateUniformBuffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count(); 

    Matrix matrix;
    matrix.viewPos = glm::vec4(0.f, 10.f, 10.f, 0.f);
    glm::mat4 model = glm::rotate(glm::mat4(1.f), 0 * glm::radians(30.f), glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 view = glm::lookAt(glm::vec3(matrix.viewPos), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), m_extent.width / (float)m_extent.height, 0.1f, 100.f);
    proj[1][1] *= -1;
    matrix.mvp = proj * view * model;
    matrix.model = model;

    glm::vec3 lightPos = glm::vec3(0.f, 3.f, 0.f);
    view = glm::lookAt(lightPos, glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));

    Mvp mvp;
    mvp.mvp = proj * view;
    matrix.lightMvp = mvp.mvp;
    memcpy(m_matrixBuffersMapped[m_curFrame], &matrix, sizeof(Matrix));

    Material material{
        .albedo = glm::vec3(1.0f, 0.71f, 0.29f),
        .metallic = 0.99f,
        .roughness = 0.7f,
    };
    memcpy(m_materialBuffersMapped[m_curFrame], &material, sizeof(material));

    Light light{
        .position = lightPos,
        .intensity = 5.f,
        .color = glm::vec3(1.f, 1.f, 1.f),
    };
    memcpy(m_lightBuffersMapped[m_curFrame], &light, sizeof(Light));

    memcpy(m_smMatrixBuffersMapped[m_curFrame], &mvp, sizeof(Mvp));
}

void PBR::createVertexBuffer()
{
    VkDeviceSize size = sizeof(MyVertex) * m_vertices.size();
    this->createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    this->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void * data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, m_vertices.data(), size);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    copyBuffer(stagingBuffer, m_vertexBuffer, size);

    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);

}

void PBR::createIndexBuffer()
{
    VkDeviceSize size = sizeof(uint32_t) * m_indices.size();
    this->createBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    this->createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void * data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, m_indices.data(), size);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    copyBuffer(stagingBuffer, m_indexBuffer, size);

    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);
}
void PBR::createSphere(glm::vec3 center, float radius, std::vector<MyVertex>& vertices, std::vector<uint32_t>& indices)
{
    constexpr int longtitude = 30;
    constexpr int latitude = 30;

    vertices.clear();
    vertices.reserve(longtitude * latitude + 2);

    glm::vec3 top = glm::vec3(center.x, center.y + radius, center.z );
    glm::vec3 bottom = glm::vec3(center.x, center.y - radius, center.z);
    vertices.emplace_back(MyVertex(top, top - center));
    
    float lon = 0;
    float lat = 0;
    constexpr float lonSlice = 3.14159265f / longtitude;
    constexpr float latSlice = 3.14159265f * 2.0f / latitude;
    for (size_t i = 1; i <= longtitude; i++)
    {
        lon = (i * lonSlice);
        float r = radius * sin(lon);
        float y = radius * cos(lon);
        for (size_t j = 0; j < latitude; j++)
        {
            lat = latSlice * j;
            float x  = r * sin(lat);
            float z = r * cos(lat);
            vertices.emplace_back(MyVertex(glm::vec3(x, y, z) + center, glm::normalize(glm::vec3(x, y, z))));
        }
        
    }
    vertices.emplace_back(MyVertex(bottom, bottom - center));
    
    indices.reserve((longtitude + 1) * (latitude + 1) * 2 + longtitude + 1);
    int t[latitude + 1] = {};
    for (int i = 0; i < latitude; i++)
    {
        t[i] = i;
    }
    
    for (uint32_t i = 0; i < longtitude + 1; i++)
    {
        for (uint32_t j = 0; j < latitude + 1; j++)
        {
            if(i == 0 )
            {
                indices.emplace_back(0);
                indices.emplace_back(1 + t[j]);
            }
            else if (i == longtitude)
            {
                indices.emplace_back(1 + (i - 1) * latitude + t[j]);
                indices.emplace_back(1 + i * latitude);
            }
            else
            {
                indices.emplace_back(1 + (i - 1) * latitude + t[j]);
                indices.emplace_back(1 + i * latitude + t[j]);
            }
        }
        indices.emplace_back(0xffffffff);
    }
}

void PBR::recordCommandBufferAndSubmit(uint32_t imageIndex)
{
    updateUniformBuffer();

    VkDescriptorImageInfo shadowMapImageInfo{
        .sampler = m_smSampler,
        .imageView = m_smImageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_descriptorSets[m_curFrame],
        .dstBinding = 3,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &shadowMapImageInfo,
    };

    vkUpdateDescriptorSets(m_logicDevice, 1, &write, 0, nullptr);


    vkResetCommandBuffer(m_commandBuffers[m_curFrame], 0);
    VkCommandBufferBeginInfo commandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
    };
    vkBeginCommandBuffer(m_commandBuffers[m_curFrame], &commandBufferBeginInfo);

    std::vector clearValue = {
        VkClearValue{
            .depthStencil = VkClearDepthStencilValue{
                .depth = 1.f,
            },
        },
    };
    VkRenderPassBeginInfo passBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_smPass,
        .framebuffer = m_smFramebuffers[imageIndex],
        .renderArea = VkRect2D{{0, 0}, {m_extent.width, m_extent.height}},
        .clearValueCount = 1,
        .pClearValues = clearValue.data(),
    };

    vkCmdBeginRenderPass(m_commandBuffers[m_curFrame], &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(m_commandBuffers[m_curFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_smPipeline);

    VkViewport viewport{0.f, 0.f, (float)m_extent.width, (float)m_extent.height, 0.f, 1.f};
    vkCmdSetViewport(m_commandBuffers[m_curFrame], 0, 1, &viewport);
    VkRect2D rect{{0, 0}, m_extent};
    vkCmdSetScissor(m_commandBuffers[m_curFrame], 0, 1, &rect);


    vkCmdBindDescriptorSets(m_commandBuffers[m_curFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_smPipelineLayout, 0, 1, &m_smDescriptorSets[m_curFrame], 0, nullptr);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_commandBuffers[m_curFrame], 0, 1, &m_vertexBuffer, offsets);
    vkCmdBindIndexBuffer(m_commandBuffers[m_curFrame], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(m_commandBuffers[m_curFrame], static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

    vkCmdBindVertexBuffers(m_commandBuffers[m_curFrame], 0, 1, &m_groundVertexBuffer, offsets);
    vkCmdBindIndexBuffer(m_commandBuffers[m_curFrame], m_groundIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(m_commandBuffers[m_curFrame], static_cast<uint32_t>(m_groundIndices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(m_commandBuffers[m_curFrame]);

    std::vector<VkClearValue> clearValues{
        VkClearValue{
            .color = {{0.0f, 0.0f, 0.0f, 1.0f}},
        },
        VkClearValue{
            .depthStencil = {1.0f, 0},
        },
    };
    
    passBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_renderPass,
        .framebuffer = m_framebuffers[imageIndex],
        .renderArea = VkRect2D{{0, 0}, {m_extent.width, m_extent.height}},
        .clearValueCount = 2,
        .pClearValues = clearValues.data(),
    };
    vkCmdBeginRenderPass(m_commandBuffers[m_curFrame], &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_commandBuffers[m_curFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    vkCmdSetViewport(m_commandBuffers[m_curFrame], 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffers[m_curFrame], 0, 1, &rect);

    vkCmdBindDescriptorSets(m_commandBuffers[m_curFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_curFrame], 0, nullptr);

    vkCmdBindVertexBuffers(m_commandBuffers[m_curFrame], 0, 1, &m_vertexBuffer, offsets);
    vkCmdBindIndexBuffer(m_commandBuffers[m_curFrame], m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(m_commandBuffers[m_curFrame], static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

    vkCmdBindVertexBuffers(m_commandBuffers[m_curFrame], 0, 1, &m_groundVertexBuffer, offsets);
    vkCmdBindIndexBuffer(m_commandBuffers[m_curFrame], m_groundIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(m_commandBuffers[m_curFrame], static_cast<uint32_t>(m_groundIndices.size()), 1, 0, 0, 0);
    
    vkCmdEndRenderPass(m_commandBuffers[m_curFrame]);

    vkEndCommandBuffer(m_commandBuffers[m_curFrame]);
    VkPipelineStageFlags waitDesStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_imageAvailabelesemaphores[m_curFrame],
        .pWaitDstStageMask = &waitDesStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_commandBuffers[m_curFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_renderFinishedSemaphores[m_curFrame],
    };

    vkQueueSubmit(m_graphicsAndComputeQueue, 1, &submitInfo, m_inFlightFence[m_curFrame]);
}

void PBR::createGround()
{
    m_groundVertices = std::vector{
        MyVertex(glm::vec3(-10.f, -2.f, -10.f), glm::vec3(0.f, 1.f, 0.f)),
        MyVertex(glm::vec3(10.f, -2.f, -10.f), glm::vec3(0.f, 1.f, 0.f)),
        MyVertex(glm::vec3(10.f, -2.f, 10.f), glm::vec3(0.f, 1.f, 0.f)),
        MyVertex(glm::vec3(-10.f, -2.f, 10.f), glm::vec3(0.f, 1.f, 0.f)),
    };
    m_groundIndices = {
        0, 3, 1, 2,
    };

    VkDeviceSize bufferSize = sizeof(MyVertex) * m_groundVertices.size();
    this->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_groundVertexBuffer, m_groundVertexBufferMemory);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    this->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void * data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_groundVertices.data(), bufferSize);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);
    copyBuffer(stagingBuffer, m_groundVertexBuffer, bufferSize);
    
    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);

    bufferSize = m_groundIndices.size() * sizeof(uint32_t);
    this->createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_groundIndexBuffer, m_groundIndexBufferMemory);
    this->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_groundIndices.data(), bufferSize);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);
    copyBuffer(stagingBuffer, m_groundIndexBuffer, bufferSize);
    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);
}

void PBR::createSMPipeline()
{
    auto vertCode = this->readFile("sm.vert.spv");
    auto fragCode = this->readFile("sm.frag.spv");
    auto vertexShader = this->createShaderModule(vertCode);
    auto fragShader = this->createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo shaderInfo []{
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
    
    VkPipelineVertexInputStateCreateInfo vertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = MyVertex::getVertexInputBinding(),
        .vertexAttributeDescriptionCount = MyVertex::getVertexInputAttributeDescriptionCount(),
        .pVertexAttributeDescriptions = MyVertex::getVertexInputAttributeDescription(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_TRUE,
    };


    VkPipelineViewportStateCreateInfo viewportInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.f,
    };
    VkPipelineMultisampleStateCreateInfo multiSampleInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineDepthStencilStateCreateInfo depthStateInfo{
        .sType =VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable =VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState attachments[] = {
        VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            
        },
    };
    VkPipelineColorBlendStateCreateInfo blendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = attachments,
    };
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

    VkGraphicsPipelineCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderInfo,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &viewportInfo,
        .pRasterizationState = &rasterInfo,
        .pMultisampleState = &multiSampleInfo,
        .pDepthStencilState = &depthStateInfo,
        .pColorBlendState = &blendState,
        .pDynamicState = &dynamicInfo,
        .layout = m_smPipelineLayout,
        .renderPass = m_smPass,
        .subpass = 0,

    };
    vkCreateGraphicsPipelines(m_logicDevice, VK_NULL_HANDLE, 1, &info, nullptr, &m_smPipeline);

    vkDestroyShaderModule(m_logicDevice, vertexShader, nullptr);
    vkDestroyShaderModule(m_logicDevice, fragShader, nullptr);   
}
void PBR::createSMSampler()
{
    VkSamplerCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0,
        .maxLod = 0,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    };
    vkCreateSampler(m_logicDevice, &info, nullptr, &m_smSampler);
}

void PBR::createCube(glm::vec3 center, float edge, std::vector<MyVertex>& vertices, std::vector<uint32_t>& indices)
{
    vertices = {
        // Front face
        MyVertex(center + glm::vec3(-edge, -edge, edge), glm::vec3(0.f, 0.f, 1.f)),
        MyVertex(center + glm::vec3(edge, -edge, edge), glm::vec3(0.f, 0.f, 1.f)),
        MyVertex(center + glm::vec3(edge, edge, edge), glm::vec3(0.f, 0.f, 1.f)),
        MyVertex(center + glm::vec3(-edge, edge, edge), glm::vec3(0.f, 0.f, 1.f)),

        // Back face
        MyVertex(center + glm::vec3(-edge, -edge, -edge), glm::vec3(0.f, 0.f, -1.f)),
        MyVertex(center + glm::vec3(edge, -edge, -edge), glm::vec3(0.f, 0.f, -1.f)),
        MyVertex(center + glm::vec3(edge, edge, -edge), glm::vec3(0.f, 0.f, -1.f)),
        MyVertex(center + glm::vec3(-edge, edge, -edge), glm::vec3(0.f, 0.f, -1.f)),

        // Top face
        MyVertex(center + glm::vec3(-edge, edge, -edge), glm::vec3(0.f, 1.f, 0.f)),
        MyVertex(center + glm::vec3(edge, edge, -edge), glm::vec3(0.f, 1.f, 0.f)),
        MyVertex(center + glm::vec3(edge, edge, edge), glm::vec3(0.f, 1.f, 0.f)),
        MyVertex(center + glm::vec3(-edge, edge, edge), glm::vec3(0.f, 1.f, 0.f)),

        // Bottom face
        MyVertex(center + glm::vec3(-edge, -edge, -edge), glm::vec3(0.f, -1.f, 0.f)),
        MyVertex(center + glm::vec3(edge, -edge, -edge), glm::vec3(0.f, -1.f, 0.f)),
        MyVertex(center + glm::vec3(edge, -edge, edge), glm::vec3(0.f, -1.f, 0.f)),
        MyVertex(center + glm::vec3(-edge, -edge, edge), glm::vec3(0.f, -1.f, 0.f)),

        // Right face
        MyVertex(center + glm::vec3(edge, -edge, -edge), glm::vec3(1.f, 0.f, 0.f)),
        MyVertex(center + glm::vec3(edge, edge, -edge), glm::vec3(1.f, 0.f, 0.f)),
        MyVertex(center + glm::vec3(edge, edge, edge), glm::vec3(1.f, 0.f, 0.f)),
        MyVertex(center + glm::vec3(edge, -edge, edge), glm::vec3(1.f, 0.f, 0.f)),

        // Left face
        MyVertex(center + glm::vec3(-edge, -edge, -edge), glm::vec3(-1.f, 0.f, 0.f)),
        MyVertex(center + glm::vec3(-edge, edge, -edge), glm::vec3(-1.f, 0.f, 0.f)),
        MyVertex(center + glm::vec3(-edge, edge, edge), glm::vec3(-1.f, 0.f, 0.f)),
        MyVertex(center + glm::vec3(-edge, -edge, edge), glm::vec3(-1.f, 0.f, 0.f)),
    };

    indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Top face
        8, 9, 10, 10, 11, 8,
        // Bottom face
        12, 13, 14, 14, 15, 12,
        // Right face
        16, 17, 18, 18, 19, 16,
        // Left face
        20, 21, 22, 22, 23, 20,
    };
}
