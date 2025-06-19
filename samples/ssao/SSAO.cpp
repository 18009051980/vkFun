#include "SSAO.h"

#include <ObjLoader.h>

#include <glm/gtc/matrix_transform.hpp>


#include <iostream>



struct UBO
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

bool SSAO::tick(float delta) 
{
    return GlfwApp::tick(delta);
}

void SSAO::loadModel(const char* filename)
{
    ObjLoader objLoader;
    objLoader.load(filename, m_vertices, m_indices);
}

void SSAO::init()
{
    createDescriptorSetLayout();
    createPipelineLayout();

    createSSAODescriptorSetLayout();
    createSSAOPipelineLayout();

    createRenderPass();
    
    createGraphicsPipeline();


    createCommandBuffer();

    loadModel("assets/objs/viking_room.obj");

    createVertexBuffer(m_vertices);
    createIndexBuffer(m_indices);
    createUniformBuffer();

    createDescriptorPool();
    
    createDepthImage();
    createTextureSampler(m_depthSampler);

    createColorResources();

    createFramebuffers();

    int width = 0;
    int height = 0;
    createTextureImage("assets/pics/viking_room.png", m_vikingroomImage, m_vikingroomImageMemory, width, height);
    
    m_vikingroomImageView = createImageView(m_vikingroomImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_mipmapLevels);
    
    createTextureSampler(m_textureSampler);

    allocateDescriptorSets();

    

}

void SSAO::clean()
{
    auto surface = m_surface;
    {
        vkDeviceWaitIdle(m_logicDevice);

        vkDestroyImageView(m_logicDevice, m_vikingroomImageView, nullptr); 
        vkDestroyImage(m_logicDevice, m_vikingroomImage, nullptr);
        vkFreeMemory(m_logicDevice, m_vikingroomImageMemory, nullptr);
        

        vkDestroySampler(m_logicDevice, m_textureSampler, nullptr);


        for (size_t i = 0; i < 2; ++i)
        {
            vkUnmapMemory(m_logicDevice, m_matrixBuffersMemory[i]);
            vkDestroyBuffer(m_logicDevice, m_matrixBuffers[i], nullptr);
            vkFreeMemory(m_logicDevice, m_matrixBuffersMemory[i], nullptr);

            vkUnmapMemory(m_logicDevice, m_timeBuffersMemory[i]);
            vkDestroyBuffer(m_logicDevice, m_timeBuffers[i], nullptr);
            vkFreeMemory(m_logicDevice, m_timeBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(m_logicDevice, m_descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(m_logicDevice, m_descriptorSetLayout, nullptr);

        vkDestroyBuffer(m_logicDevice, m_vertexBuffer, nullptr);
        vkFreeMemory(m_logicDevice, m_vertexBufferMemory, nullptr);

        vkDestroyBuffer(m_logicDevice, m_indexBuffer, nullptr);
        vkFreeMemory(m_logicDevice, m_indexBufferMemory, nullptr);

        vkFreeCommandBuffers(m_logicDevice, m_commandPool, (uint32_t)m_commandBuffers.size(), m_commandBuffers.data());

        vkDestroyPipeline(m_logicDevice, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_logicDevice, m_pipelineLayout, nullptr);

        vkDestroyRenderPass(m_logicDevice, m_renderPass, nullptr);
    }
    
    
}

void SSAO::createVertexBuffer(const std::vector<MyVertex>& vertices) 
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

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);
    
    copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);

}


void SSAO::createIndexBuffer(const std::vector<uint32_t>& indices)
{
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

    copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);

}


void SSAO::createDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    };
 
    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    if (VK_SUCCESS != vkCreateDescriptorSetLayout(m_logicDevice, &layoutInfo, nullptr, &m_descriptorSetLayout))
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

}


void SSAO::allocateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(2, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo alloInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = (uint32_t)layouts.size(),
        .pSetLayouts = layouts.data(),
    };
    m_descriptorSets.resize(2);

    if (VK_SUCCESS != vkAllocateDescriptorSets(m_logicDevice, &alloInfo, m_descriptorSets.data()))
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    std::vector<VkDescriptorBufferInfo> bufferInfos{
        VkDescriptorBufferInfo{
            .buffer = m_matrixBuffers[0],
            .offset = 0,
            .range = sizeof(UBO),
        },
        VkDescriptorBufferInfo{
            .buffer = m_matrixBuffers[1],
            .offset = 0,
            .range = sizeof(UBO),
        },
        VkDescriptorBufferInfo{
            .buffer = m_timeBuffers[1],
            .offset = 0,
            .range = sizeof(float),
        },
        VkDescriptorBufferInfo{
            .buffer = m_timeBuffers[1],
            .offset = 0,
            .range = sizeof(float),
        },
    };
    std::vector<VkDescriptorImageInfo> imageInfo{
        VkDescriptorImageInfo{
            .sampler = m_textureSampler,
            .imageView = m_vikingroomImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
    };
    std::vector<VkWriteDescriptorSet> descriptorWrites{
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[0],
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
            .dstSet = m_descriptorSets[1],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfos[1],
            .pTexelBufferView = nullptr
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[0],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfos[2],
            .pTexelBufferView = nullptr
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[1],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfos[3],
            .pTexelBufferView = nullptr
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[0],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = imageInfo.data(),
        },
        VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[1],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = imageInfo.data(),
        },
    };
    vkUpdateDescriptorSets(m_logicDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}



void SSAO::createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 4,   //matrix buffer, timebuffer
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 2 + 2, //modelImage, ssao depthImage
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 6,    //ssao positionImage, normalImage, outpuImage
        }
    };

    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 2,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };


    if (VK_SUCCESS != vkCreateDescriptorPool(m_logicDevice, &poolInfo, nullptr, & m_descriptorPool))
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }

}

void SSAO::createUniformBuffer()
{
    m_matrixBuffers.resize(2);
    m_matrixBuffersMemory.resize(2);
    m_matrixBuffersMapped.resize(2);
    VkDeviceSize bufferSize = sizeof(UBO);
    for (size_t i = 0; i < 2; i++)
    {
        createBuffer(bufferSize,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_matrixBuffers[i], m_matrixBuffersMemory[i]);
        
        vkMapMemory(m_logicDevice, m_matrixBuffersMemory[i], 0, bufferSize, 0, &m_matrixBuffersMapped[i]);

    }

    m_timeBuffers.resize(2);
    m_timeBuffersMemory.resize(2);
    m_timeBuffersMapped.resize(2);

    VkDeviceSize timeBufferSize = sizeof(float);
    for (size_t i = 0; i < 2; i++)
    {
        createBuffer(timeBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_timeBuffers[i], m_timeBuffersMemory[i]);
        
        vkMapMemory(m_logicDevice, m_timeBuffersMemory[i], 0, timeBufferSize, 0, &m_timeBuffersMapped[i]);
    }   
}


void SSAO::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("ssao.vert.spv");
    auto fragShaderCode = readFile("ssao.frag.spv");

    auto vertShaderModule = createShaderModule(vertShaderCode);
    auto fragShaderModule = createShaderModule(fragShaderCode);
    

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = MyVertex::getVertexInputBinding();
    vertexInputInfo.vertexAttributeDescriptionCount = MyVertex::getVertexInputAttributeDescriptionCount();
    vertexInputInfo.pVertexAttributeDescriptions = MyVertex::getVertexInputAttributeDescription();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;


    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = 0.2f; // Optional
    VkPipelineDepthStencilStateCreateInfo depthCreateInfo{
        .sType =VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable =VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment[] = 
    {
        VkPipelineColorBlendAttachmentState{
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        },
        VkPipelineColorBlendAttachmentState{
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        },
    };
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 2;
    colorBlending.pAttachments = colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = (uint32_t)dynamicStates.size();
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    


    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthCreateInfo; 
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(m_logicDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(m_logicDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_logicDevice, fragShaderModule, nullptr);
}


void SSAO::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
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
            .depthStencil = {1.0f, 0},
        },
        VkClearValue{
            .color = {{.0f, 0.0f, 0.0f, 1.0f}},
        },
        VkClearValue{
            .color = {{0.0f, 0.0f, 0.0f, 1.0f}},
        },
    };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
   
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkBuffer vertexBuffers [] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    updateUniformBuffer();

    VkViewport viewport{0.f, 0.f, (float)m_extent.width, (float)m_extent.height, 0.f, 1.f};

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, m_extent};

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_curFrame], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to record command buffer!");
    }

}


void SSAO::updateUniformBuffer()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();    

    UBO ubo = {
        .model = glm::rotate(glm::mat4(1.f), 0 * glm::radians(30.f), glm::vec3(0.f, 0.f, 1.f)),
        .view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = glm::perspective(glm::radians(45.0f), m_extent.width / (float) m_extent.height, 1.f, 10.0f)
    };
    ubo.proj[1][1] *= -1.f;

    memcpy(m_matrixBuffersMapped[m_curFrame], &ubo, sizeof(ubo));

    memcpy(m_timeBuffersMapped[m_curFrame], &time, sizeof(float));
}

void SSAO::drawFrame()
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

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailabelesemaphores[m_curFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_curFrame];

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_curFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_graphicsAndComputeQueue, 1, &submitInfo, m_inFlightFence[m_curFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

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
    m_curFrame = (m_curFrame + 1) & 1;
}


void SSAO::createRenderPass()
{
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    
    std::vector<VkAttachmentDescription> attachments{
        VkAttachmentDescription{
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
        VkAttachmentDescription{
            .format = m_GBufferImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        VkAttachmentDescription{
            .format = m_GBufferImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };
    
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    
    std::vector subpassAttachments{
        VkAttachmentReference{
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        VkAttachmentReference{
            .attachment = 2,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };
    VkAttachmentReference depthRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    std::vector subpasses = {
        VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = static_cast<uint32_t>(subpassAttachments.size()),
            .pColorAttachments = subpassAttachments.data(),
            .pDepthStencilAttachment = &depthRef,
        }
    };
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassInfo.pSubpasses = subpasses.data();

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_logicDevice, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void SSAO::createFramebuffers()
{
    m_framebuffers.resize(m_depthImageView.size());
    for (size_t i = 0; i < m_depthImageView.size(); i++) 
    {
        std::vector<VkImageView> attachments = {
            m_depthImageView[i],
            m_positionImageView[i],
            m_normalImageView[i],
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_extent.width;
        framebufferInfo.height = m_extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_logicDevice, &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void SSAO::createCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 2;
    m_commandBuffers.resize(2);

    if (vkAllocateCommandBuffers(m_logicDevice, &allocInfo, &m_commandBuffers[0]) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}



void SSAO::createTextureImage(const char* filename, VkImage &image, VkDeviceMemory& imageMemory, int& width, int& height)
{
    int channels;
    auto pixels = ImageLoader::loadImage(filename, &width, &height, &channels);

    VkDeviceSize imageSize = width * height * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    m_mipmapLevels = static_cast<int>(std::log2(std::max(width, height))) + 1;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    void * stagingBufferMapped = nullptr;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    vkMapMemory(m_logicDevice, stagingBufferMemory, 0, imageSize, 0, &stagingBufferMapped);
    memcpy(stagingBufferMapped, pixels, imageSize);
    vkUnmapMemory(m_logicDevice, stagingBufferMemory);

    ImageLoader::freeImage(pixels);
    pixels = nullptr;

    createImage(width, height, m_mipmapLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    VkCommandBuffer commandBuffer{};
    allocateCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    transitionImageLayout(
        commandBuffer,
        image, 
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        m_mipmapLevels);

    copyBufferToImage(commandBuffer, stagingBuffer, image, width, height);

    genMipmaps(commandBuffer, image, width, height, m_mipmapLevels);

    // transitionImageLayout(
    //     commandBuffer,
    //     image, 
    //     VK_FORMAT_R8G8B8A8_SRGB, 
    //     VK_IMAGE_ASPECT_COLOR_BIT,
    //     VK_ACCESS_TRANSFER_WRITE_BIT,
    //     VK_ACCESS_SHADER_READ_BIT,
    //     VK_PIPELINE_STAGE_TRANSFER_BIT,
    //     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //     m_mipmapLevels);
    
    endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);


    vkDestroyBuffer(m_logicDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_logicDevice, stagingBufferMemory, nullptr);

}

void SSAO::createDepthImage()
{
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    for (size_t i = 0; i < 2; i++)
    {
        createImage(m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage[i], m_depthImageMemory[i]);
        m_depthImageView[i] = createImageView(m_depthImage[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    }
    

    
    VkCommandBuffer commandBuffer;
    allocateCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    for (size_t i = 0; i < m_depthImage.size(); i++)
    {
        transitionImageLayout(
            commandBuffer,
            m_depthImage[i], depthFormat, 
            VK_IMAGE_ASPECT_DEPTH_BIT,
            0, 
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            1);
    }
    endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);
}


void SSAO::cleanupDepthImage()
{
    for (size_t i = 0; i < m_depthImage.size(); i++)
    {
        vkDestroyImageView(m_logicDevice, m_depthImageView[i], nullptr);
        vkDestroyImage(m_logicDevice, m_depthImage[i], nullptr);
        vkFreeMemory(m_logicDevice, m_depthImageMemory[i], nullptr);
    }
}

void SSAO::createColorResources()
{
    for (size_t i = 0; i < m_positionImage.size(); i++)
    {
        createImage(m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_GBufferImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_positionImage[i], m_positionImageMemory[i]);

        m_positionImageView[i] = createImageView(m_positionImage[i], m_GBufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

        createImage(m_extent.width, m_extent.height, 1, VK_SAMPLE_COUNT_1_BIT, m_GBufferImageFormat, VK_IMAGE_TILING_OPTIMAL,   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_normalImage[i], m_normalImageMemory[i]);
        m_normalImageView[i] = createImageView(m_normalImage[i], m_GBufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
    
    VkCommandBuffer commandBuffer;
    allocateCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    for (size_t i = 0; i < m_positionImage.size(); i++)
    {
        transitionImageLayout(
            commandBuffer,
            m_positionImage[i], m_GBufferImageFormat, 
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT  | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ,
            1);
        transitionImageLayout(
            commandBuffer,
            m_normalImage[i], m_GBufferImageFormat, 
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT  | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ,
            1);
    }
    endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);
}

void SSAO::cleanupColorResources()
{
    for (size_t i = 0; i < m_framesInFlight; i++)
    {
        vkDestroyImageView(m_logicDevice, m_positionImageView[i], nullptr);
        vkDestroyImage(m_logicDevice, m_positionImage[i], nullptr);
        vkFreeMemory(m_logicDevice, m_positionImageMemory[i], nullptr);

        vkDestroyImageView(m_logicDevice, m_normalImageView[i], nullptr);
        vkDestroyImage(m_logicDevice, m_normalImage[i], nullptr);
        vkFreeMemory(m_logicDevice, m_normalImageMemory[i], nullptr);
    }
}

void SSAO::createPipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout; // Optional

    if (vkCreatePipelineLayout(m_logicDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void SSAO::createSSAODescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
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
    };
    VkDescriptorSetLayoutCreateInfo ssaoDescriptorSetLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };
    
    vkCreateDescriptorSetLayout(m_logicDevice, &ssaoDescriptorSetLayoutInfo, nullptr, &m_ssaoDescriptorSetLayout);   
}

void SSAO::createSSAOPipelineLayout()
{
    VkPipelineLayoutCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_ssaoDescriptorSetLayout,
    };
    vkCreatePipelineLayout(m_logicDevice, &info, nullptr, &m_ssaoPipelineLayout);
}