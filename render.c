#include <time.h>
#include "render.h"

VCW_Renderpass create_rendp(VCW_Device vcw_dev, VCW_Surface surf) {
    VCW_Renderpass vcw_rendp;
    vcw_rendp.frame_bufs = NULL;
    vcw_rendp.frame_buf_count = 0;
    vcw_rendp.targets = NULL;
    vcw_rendp.target_count = 0;
    //
    // fill attachment description
    //
    VkAttachmentDescription attach_desc;
    attach_desc.flags = 0;
    attach_desc.format = surf.forms[0].format;
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    attach_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attach_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attach_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attach_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attach_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attach_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    printf("attachment description filled.\n");
    //
    // fill attachment reference
    //
    VkAttachmentReference attach_ref;
    attach_ref.attachment = 0;
    attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    printf("attachment reference filled.\n");
    //
    // fill subpass description
    //
    VkSubpassDescription subp_desc;
    subp_desc.flags = 0;
    subp_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subp_desc.inputAttachmentCount = 0;
    subp_desc.pInputAttachments = NULL;
    subp_desc.colorAttachmentCount = 1;
    subp_desc.pColorAttachments = &attach_ref;
    subp_desc.pResolveAttachments = NULL;
    subp_desc.pDepthStencilAttachment = NULL;
    subp_desc.preserveAttachmentCount = 0;
    subp_desc.pPreserveAttachments = NULL;
    printf("subpass description filled.\n");
    //
    // fill subpass dependency
    //
    VkSubpassDependency subp_dep;
    subp_dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    subp_dep.dstSubpass = 0;
    subp_dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subp_dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subp_dep.srcAccessMask = 0;
    subp_dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subp_dep.dependencyFlags = 0;
    printf("subpass dependency created.\n");
    //
    // create render pass
    //
    VkRenderPassCreateInfo rendp_info;
    rendp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rendp_info.pNext = NULL;
    rendp_info.flags = 0;
    rendp_info.attachmentCount = 1;
    rendp_info.pAttachments = &attach_desc;
    rendp_info.subpassCount = 1;
    rendp_info.pSubpasses = &subp_desc;
    rendp_info.dependencyCount = 1;
    rendp_info.pDependencies = &subp_dep;

    vkCreateRenderPass(vcw_dev.dev, &rendp_info, NULL, &vcw_rendp.rendp);
    printf("render pass created.\n");
    return vcw_rendp;
}

VCW_Buffer
create_index_buf(VCW_Device vcw_dev, VCW_PhysicalDevice vcw_phy_dev, uint32_t *indices, uint32_t num_indices) {
    VCW_Buffer index_buf = create_buffer(vcw_dev, num_indices * sizeof(uint32_t), sizeof(uint32_t),
                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);

    allocate_memory(vcw_dev, vcw_phy_dev, &index_buf);
    copy_data_to_buf(vcw_dev, &index_buf, indices);

    printf("index buffer created.\n");

    return index_buf;
}

VCW_Buffer
create_vertex_buf(VCW_Device vcw_dev, VCW_PhysicalDevice vcw_phy_dev, Vertex *vertices, uint32_t num_vertices) {
    VCW_Buffer vert_buf = create_buffer(vcw_dev, num_vertices * sizeof(Vertex), sizeof(struct Vertex),
                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);

    allocate_memory(vcw_dev, vcw_phy_dev, &vert_buf);
    copy_data_to_buf(vcw_dev, &vert_buf, vertices);

    printf("vertex buffer created.\n");
    return vert_buf;
}

VCW_Buffer *create_unif_bufs(VCW_Device vcw_dev, VCW_PhysicalDevice vcw_phy_dev, uint32_t unif_buf_count) {
    VCW_Buffer *unif_bufs = malloc(unif_buf_count * sizeof(VCW_Buffer));
    for (uint32_t i = 0; i < unif_buf_count; i++) {
        unif_bufs[i] = create_buffer(vcw_dev, sizeof(VCW_Uniform), sizeof(VCW_Uniform),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);

        allocate_memory(vcw_dev, vcw_phy_dev, &unif_bufs[i]);
        map_mem(vcw_dev, &unif_bufs[i]);
    }

    printf("uniform buffers created.\n");
    return unif_bufs;
}

VCW_Pipeline create_pipe(VCW_Device vcw_dev, VCW_Renderpass rendp, VCW_DescriptorPool vcw_desc) {
    VCW_Pipeline vcw_pipe;
    //
    // load shader
    //
    FILE *fp_vert = NULL, *fp_frag = NULL;
    if (fopen_s(&fp_vert, "vert.spv", "rb+") != 0)
        printf("cannot find vertex shader code.\n");
    if (fopen_s(&fp_frag, "frag.spv", "rb+") != 0)
        printf("cannot find fragment shader code.\n");
    char shader_loaded = 1;
    if (fp_vert == NULL || fp_frag == NULL) {
        shader_loaded = 0;
        printf("cannot find shader code.\n");
    }
    fseek(fp_vert, 0, SEEK_END);
    fseek(fp_frag, 0, SEEK_END);
    uint32_t vert_size = ftell(fp_vert);
    uint32_t frag_size = ftell(fp_frag);

    char *p_vert_code = (char *) malloc(vert_size * sizeof(char));
    char *p_frag_code = (char *) malloc(frag_size * sizeof(char));

    rewind(fp_vert);
    rewind(fp_frag);
    fread(p_vert_code, 1, vert_size, fp_vert);
    printf("vertex shader binaries loaded.\n");
    fread(p_frag_code, 1, frag_size, fp_frag);
    printf("fragment shader binaries loaded.\n");

    fclose(fp_vert);
    fclose(fp_frag);
    //
    // create shader modules
    //
    VkShaderModuleCreateInfo vert_shad_mod_info;
    vert_shad_mod_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_shad_mod_info.pNext = NULL;
    vert_shad_mod_info.flags = 0;
    vert_shad_mod_info.codeSize = shader_loaded ? vert_size : 0;
    vert_shad_mod_info.pCode = shader_loaded ? (const uint32_t *) p_vert_code : NULL;

    VkShaderModuleCreateInfo frag_shad_mod_info;
    frag_shad_mod_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_shad_mod_info.pNext = NULL;
    frag_shad_mod_info.flags = 0;
    frag_shad_mod_info.codeSize = shader_loaded ? frag_size : 0;
    frag_shad_mod_info.pCode = shader_loaded ? (const uint32_t *) p_frag_code : NULL;

    VkShaderModule vert_shad_mod, frag_shad_mod;
    vkCreateShaderModule(vcw_dev.dev, &vert_shad_mod_info, NULL, &vert_shad_mod);
    printf("vertex shader module created.\n");
    vkCreateShaderModule(vcw_dev.dev, &frag_shad_mod_info, NULL, &frag_shad_mod);
    printf("fragment shader module created.\n");
    //
    // fill shader stage info
    //
    VkPipelineShaderStageCreateInfo vert_stage_info, frag_stage_info, stage_infos[2];

    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.pNext = NULL;
    vert_stage_info.flags = 0;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_shad_mod;
    char vert_entry[VK_MAX_EXTENSION_NAME_SIZE];
    strcpy_s(vert_entry, VK_MAX_EXTENSION_NAME_SIZE, "main");
    vert_stage_info.pName = vert_entry;
    vert_stage_info.pSpecializationInfo = NULL;
    printf("vertex shader stage info filled.\n");

    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.pNext = NULL;
    frag_stage_info.flags = 0;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_shad_mod;
    char frag_entry[VK_MAX_EXTENSION_NAME_SIZE];
    strcpy_s(frag_entry, VK_MAX_EXTENSION_NAME_SIZE, "main");
    frag_stage_info.pName = frag_entry;
    frag_stage_info.pSpecializationInfo = NULL;
    printf("fragment shader stage info filled.\n");

    stage_infos[0] = vert_stage_info;
    stage_infos[1] = frag_stage_info;
    printf("created shader stages.\n");
    //
    //create vertex input binding description
    //
    uint32_t vert_bindg_desc_count = 1;
    VkVertexInputBindingDescription *vert_bindg_descs = malloc(
            vert_bindg_desc_count * sizeof(VkVertexInputBindingDescription));
    vert_bindg_descs[0].binding = 0;
    vert_bindg_descs[0].stride = sizeof(struct Vertex);
    vert_bindg_descs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    printf("vertex input binding description created.\n");
    //
    // create vertex attribute description
    // !!! ADJUST TO YOUR VERTEX STRUCT !!!
    //
    uint32_t vert_attr_desc_count = 2;
    VkVertexInputAttributeDescription *vert_attr_descs = malloc(
            vert_attr_desc_count * sizeof(VkVertexInputAttributeDescription));
    vert_attr_descs[0].binding = 0;
    vert_attr_descs[0].location = 0;
    vert_attr_descs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vert_attr_descs[0].offset = offsetof(struct Vertex, pos);
    printf("vertex attribute description created.\n");
    vert_attr_descs[1].binding = 0;
    vert_attr_descs[1].location = 1;
    vert_attr_descs[1].format = VK_FORMAT_R32G32_SFLOAT;
    vert_attr_descs[1].offset = offsetof(struct Vertex, uv);
    printf("vertex attribute description created.\n");
    //
    // fill vertex input state info
    //
    VkPipelineVertexInputStateCreateInfo vert_input_info;
    vert_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_input_info.pNext = NULL;
    vert_input_info.flags = 0;
    vert_input_info.vertexBindingDescriptionCount = vert_bindg_desc_count;
    vert_input_info.pVertexBindingDescriptions = vert_bindg_descs;
    vert_input_info.vertexAttributeDescriptionCount = vert_attr_desc_count;
    vert_input_info.pVertexAttributeDescriptions = vert_attr_descs;
    printf("vertex input state info filled.\n");
    //
    // fill input assembly state info
    //
    VkPipelineInputAssemblyStateCreateInfo input_asm_info;
    input_asm_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm_info.pNext = NULL;
    input_asm_info.flags = 0;
    input_asm_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_asm_info.primitiveRestartEnable = VK_FALSE;
    printf("input assembly info filled.\n");
    //
    // fill viewport state info
    //
    VkPipelineViewportStateCreateInfo vwp_state_info;
    vwp_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vwp_state_info.pNext = NULL;
    vwp_state_info.flags = 0;
    vwp_state_info.viewportCount = 1;
    // vwp_state_info.pViewports = &viewport;
    vwp_state_info.scissorCount = 1;
    // vwp_state_info.pScissors = &scissor;
    printf("viewport state filled.\n");
    //
    // create dynamic states
    //
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn_state_info;
    dyn_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_state_info.pNext = NULL;
    dyn_state_info.flags = 0;
    dyn_state_info.dynamicStateCount = 2;
    dyn_state_info.pDynamicStates = dynamic_states;
    printf("dynamic states created.\n");
    //
    // fill rasterizer state info
    //
    VkPipelineRasterizationStateCreateInfo rast_info;
    rast_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast_info.pNext = NULL;
    rast_info.flags = 0;
    rast_info.depthClampEnable = VK_FALSE;
    rast_info.rasterizerDiscardEnable = VK_FALSE;
    // CHANGE FILL / BORDER
    rast_info.polygonMode = VK_POLYGON_MODE_FILL;
    // ACTIVATE / DEACTIVATE CULLING
    rast_info.cullMode = VK_CULL_MODE_BACK_BIT;
    // CHANGE YOUR POLYGON ORDER -> CORRECT ORDER FOR CULLING
    rast_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // STORE DEPTH INFORMATION (available in v2)
    rast_info.depthBiasEnable = VK_FALSE;
    rast_info.depthBiasConstantFactor = 0.0f;
    rast_info.depthBiasClamp = 0.0f;
    rast_info.depthBiasSlopeFactor = 0.0f;
    rast_info.lineWidth = 1.0f;
    printf("rasterization info filled.\n");
    //
    // fill multisampling state info
    //
    VkPipelineMultisampleStateCreateInfo samp_info;
    samp_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    samp_info.pNext = NULL;
    samp_info.flags = 0;
    samp_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    samp_info.sampleShadingEnable = VK_FALSE;
    samp_info.minSampleShading = 1.0f;
    samp_info.pSampleMask = NULL;
    samp_info.alphaToCoverageEnable = VK_FALSE;
    samp_info.alphaToOneEnable = VK_FALSE;
    printf("multisample info filled.\n");
    //
    // fill color blend attachment state
    // - could be used for jfa later
    // - just create a new pipeline with blending active (efficiency?)
    VkPipelineColorBlendAttachmentState color_blend_attach;
    color_blend_attach.blendEnable = VK_FALSE;
    color_blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attach.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attach.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attach.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    printf("color blend attachment state filled.\n");
    //
    // fill color blend state info
    //
    VkPipelineColorBlendStateCreateInfo color_blend_info;
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.pNext = NULL;
    color_blend_info.flags = 0;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attach;
    for (uint32_t i = 0; i < 4; i++) {
        color_blend_info.blendConstants[i] = 0.0f;
    }
    printf("color blend state info filled.\n");
    //
    // create push constant range
    //
    VkPushConstantRange push_const_range;
    push_const_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    push_const_range.offset = 0;
    push_const_range.size = sizeof(VCW_PushConstant);
    //
    // create pipeline layout
    //
    VkPipelineLayoutCreateInfo pipe_layout_info;
    pipe_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipe_layout_info.pNext = NULL;
    pipe_layout_info.flags = 0;
    pipe_layout_info.setLayoutCount = 0; // vcw_desc.set_count;
    pipe_layout_info.pSetLayouts = NULL; // vcw_desc.layouts;
    pipe_layout_info.pushConstantRangeCount = 0; // 1;
    pipe_layout_info.pPushConstantRanges = NULL; // &push_const_range;

    vkCreatePipelineLayout(vcw_dev.dev, &pipe_layout_info, NULL, &vcw_pipe.layout);
    printf("pipeline layout created.\n");
    //
    // create pipeline
    //
    VkGraphicsPipelineCreateInfo pipe_info;
    pipe_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipe_info.pNext = NULL;
    pipe_info.flags = 0;
    pipe_info.stageCount = 2;
    pipe_info.pStages = stage_infos;
    pipe_info.pVertexInputState = &vert_input_info;
    pipe_info.pInputAssemblyState = &input_asm_info;
    pipe_info.pTessellationState = NULL;
    pipe_info.pViewportState = &vwp_state_info;
    pipe_info.pRasterizationState = &rast_info;
    pipe_info.pMultisampleState = &samp_info;
    pipe_info.pDepthStencilState = NULL;
    pipe_info.pColorBlendState = &color_blend_info;
    pipe_info.pDynamicState = &dyn_state_info;

    pipe_info.layout = vcw_pipe.layout;
    pipe_info.renderPass = rendp.rendp;
    pipe_info.subpass = 0;
    pipe_info.basePipelineHandle = VK_NULL_HANDLE;
    pipe_info.basePipelineIndex = -1;

    vkCreateGraphicsPipelines(vcw_dev.dev, VK_NULL_HANDLE, 1, &pipe_info, NULL, &vcw_pipe.pipe);
    printf("graphics pipeline created.\n");
    //
    // destroy shader module
    //
    vkDestroyShaderModule(vcw_dev.dev, frag_shad_mod, NULL);
    printf("fragment shader module destroyed.\n");
    vkDestroyShaderModule(vcw_dev.dev, vert_shad_mod, NULL);
    printf("vertex shader module destroyed.\n");
    free(p_frag_code);
    printf("fragment shader binaries released.\n");
    free(p_vert_code);
    printf("vertex shader binaries released.\n");
    return vcw_pipe;
}

void create_render_targets(VCW_Device vcw_dev, VCW_PhysicalDevice vcw_phy_dev, VCW_Surface vcw_surf,
                           VCW_Renderpass *vcw_rendp, uint32_t target_count, VkExtent2D extent) {
    vcw_rendp->target_count = target_count;
    vcw_rendp->targets = malloc(vcw_rendp->target_count * sizeof(VCW_Image));
    for (uint32_t i = 0; i < vcw_rendp->target_count; i++) {
        vcw_rendp->targets[i] = create_img(vcw_dev, extent, vcw_surf.forms[0].format,
                                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        create_img_sampler(vcw_dev, &(vcw_rendp->targets[i]), VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
        create_img_memory(vcw_dev, vcw_phy_dev, &(vcw_rendp->targets[i]));
        create_img_view(vcw_dev, &(vcw_rendp->targets[i]));
    }
    printf("render targets created.\n");
}

void create_frame_bufs(VCW_Device vcw_dev, VCW_Renderpass *vcw_rendp, uint32_t frame_buf_count, VkImageView *attachs,
                       VkExtent2D extent) {
    //
    // create framebuffers
    //
    vcw_rendp->frame_buf_count = frame_buf_count;
    VkFramebufferCreateInfo *frame_buf_infos = malloc(vcw_rendp->frame_buf_count * sizeof(VkFramebufferCreateInfo));
    vcw_rendp->frame_bufs = malloc(vcw_rendp->frame_buf_count * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vcw_rendp->frame_buf_count; i++) {
        frame_buf_infos[i].sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frame_buf_infos[i].pNext = NULL;
        frame_buf_infos[i].flags = 0;
        frame_buf_infos[i].renderPass = vcw_rendp->rendp;
        frame_buf_infos[i].attachmentCount = 1;
        frame_buf_infos[i].pAttachments = &(attachs[i]);
        frame_buf_infos[i].width = extent.width;
        frame_buf_infos[i].height = extent.height;
        frame_buf_infos[i].layers = 1;

        vkCreateFramebuffer(vcw_dev.dev, &(frame_buf_infos[i]), NULL, &(vcw_rendp->frame_bufs[i]));
        printf("framebuffer %d created.\n", i);
    }
}

// the basic layout for this is that there are
// two frames in flight with total 3 swapchain images
// so only two of them will be rendered at same time
VCW_Sync create_sync(VCW_Device vcw_dev, uint32_t img_count, uint32_t max_frames_in_flight) {
    VCW_Sync vcw_sync;
    // vcw_sync.img_count = img_count;
    vcw_sync.cur_frame = 0;
    // THESE ARE MAX FRAMES IN FLIGHT
    vcw_sync.max_frames = max_frames_in_flight;
    //
    // semaphore and fence creation
    //
    vcw_sync.img_avl_semps = malloc(vcw_sync.max_frames * sizeof(VkSemaphore));
    vcw_sync.rend_fin_semps = malloc(vcw_sync.max_frames * sizeof(VkSemaphore));
    vcw_sync.fens = malloc(vcw_sync.max_frames * sizeof(VkFence));

    VkSemaphoreCreateInfo semp_info;
    semp_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semp_info.pNext = NULL;
    semp_info.flags = 0;

    VkFenceCreateInfo fen_info;
    fen_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fen_info.pNext = NULL;
    fen_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < vcw_sync.max_frames; i++) {
        vkCreateSemaphore(vcw_dev.dev, &semp_info, NULL, &(vcw_sync.img_avl_semps[i]));
        vkCreateSemaphore(vcw_dev.dev, &semp_info, NULL, &(vcw_sync.rend_fin_semps[i]));
        vkCreateFence(vcw_dev.dev, &fen_info, NULL, &(vcw_sync.fens[i]));
    }
    printf("semaphores and fences created.\n");

    vcw_sync.cur_frame = 0;

    /*
    vcw_sync.img_fens = malloc(vcw_sync.img_count * sizeof(VkFence));
    for (uint32_t i = 0; i < vcw_sync.img_count; i++) {
        vcw_sync.img_fens[i] = VK_NULL_HANDLE;
    }
    */

    return vcw_sync;
}

void first_img_memory_barriers(VCW_Image img) {
    VkImageMemoryBarrier write;
    write.image = img.img;
    write.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    write.subresourceRange = DEFAULT_SUBRESOURCE_RANGE;
    write.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    write.newLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkImageMemoryBarrier transfer;
    write.image = img.img;
    write.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    write.subresourceRange = DEFAULT_SUBRESOURCE_RANGE;
    write.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    write.newLayout = VK_IMAGE_LAYOUT_GENERAL;
}

// MODIFY THIS FUNCTION TO YOUR RENDERING NEEDS
void record_cmd_buf(VCW_VkCoreGroup vcw_core, VCW_App vcw_app, VkCommandBuffer cmd_buf, uint32_t img_index,
                    uint32_t cur_frame) {
    //
    // unpack arguments
    //
    VCW_Swapchain vcw_swap = *vcw_core.swap;
    VCW_Renderpass vcw_rendp = *vcw_app.rendp;
    VCW_Pipeline vcw_pipe = *vcw_app.pipe;
    VCW_DescriptorPool vcw_desc = *vcw_app.desc;
    VCW_Buffer vert_buf = *vcw_app.vert_buf;
    VCW_Buffer index_buf = *vcw_app.index_buf;
    //
    // record command buffer
    //
    // begin command buffer
    //
    VkCommandBufferBeginInfo cmd_begin_info;
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext = NULL;
    cmd_begin_info.flags = 0;
    cmd_begin_info.pInheritanceInfo = NULL;
    vkBeginCommandBuffer(cmd_buf, &cmd_begin_info);
    //
    // begin renderpass
    //
    VkRect2D rendp_area;
    rendp_area.offset.x = 0;
    rendp_area.offset.y = 0;
    rendp_area.extent = vcw_swap.extent;
    VkClearValue clear_val = {0.0f, 0.0f, 0.0f, 0.0f};

    VkRenderPassBeginInfo rendp_begin_info;
    rendp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rendp_begin_info.pNext = NULL;
    rendp_begin_info.renderPass = vcw_rendp.rendp;
    rendp_begin_info.framebuffer = vcw_rendp.frame_bufs[img_index];
    rendp_begin_info.renderArea = rendp_area;
    rendp_begin_info.clearValueCount = 1;
    rendp_begin_info.pClearValues = &clear_val;

    vkCmdBeginRenderPass(cmd_buf, &(rendp_begin_info), VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vcw_pipe.pipe);

    //
    // fill viewport
    //
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f; //(float) vcw_swap.extent.height;
    viewport.width = (float) vcw_swap.extent.width;
    viewport.height = (float) vcw_swap.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
    //
    // fill scissor
    //
    VkRect2D scissor;
    VkOffset2D sci_offset;
    sci_offset.x = 0;
    sci_offset.y = 0;
    scissor.offset = sci_offset;
    scissor.extent = vcw_swap.extent;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vert_buf.buf, &(VkDeviceSize) {0});
    vkCmdBindIndexBuffer(cmd_buf, index_buf.buf, 0, VK_INDEX_TYPE_UINT32);

    // VCW_Buffer unif_buf = vcw_app.unif_bufs[cur_frame];
    // memcpy(unif_buf.cpu_mem_pointer, vcw_app.cpu_unif, unif_buf.size);

    /*
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vcw_pipe.layout, 0, vcw_desc.set_count,
                            vcw_desc.sets, 0, NULL);
    vkCmdPushConstants(cmd_buf, vcw_pipe.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof(VCW_PushConstant), vcw_app.cpu_push_const);
    */

    vkCmdDrawIndexed(cmd_buf, vcw_app.num_indices, 1, 0, 0, 0);
    vkCmdDraw(cmd_buf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmd_buf);
    vkEndCommandBuffer(cmd_buf);
}

VCW_RenderResult render(VCW_VkCoreGroup vcw_core, VCW_App vcw_app) {
    clock_t render_start = clock();
    clock_t start;
    clock_t end;
    //
    // unpack group arguments
    //
    VCW_Device vcw_dev = *vcw_core.dev;
    VCW_Swapchain *vcw_swap = vcw_core.swap;
    uint32_t cur_frame = vcw_app.sync->cur_frame;
    VkFence fence = vcw_app.sync->fens[cur_frame];
    VkSemaphore img_avl_semp = vcw_app.sync->img_avl_semps[cur_frame];
    VkSemaphore rend_fin_semp = vcw_app.sync->rend_fin_semps[cur_frame];
    //
    // main rendering
    //
    // vkWaitForFences(vcw_dev.dev, 1, &(fence), VK_TRUE, UINT64_MAX);

    uint32_t img_index = 0;
    VkResult result = vkAcquireNextImageKHR(vcw_dev.dev, vcw_swap->swap, UINT64_MAX, img_avl_semp,
                                            VK_NULL_HANDLE, &img_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap(vcw_core, vcw_app);
        return 2;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        printf("failed to acquire next image.\n");
        return 1;
    }

    VkCommandBuffer cmd_buf = vcw_app.cmd->cmd_bufs[img_index];
    vkResetFences(vcw_dev.dev, 1, &(fence));

    vkResetCommandBuffer(cmd_buf, 0);
    start = clock();
    record_cmd_buf(vcw_core, vcw_app, cmd_buf, img_index, cur_frame);
    end = clock();
    vcw_app.stats->cmd_record_time = (end - start) / (double) CLOCKS_PER_SEC;

    VkSubmitInfo sub_info;
    sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    sub_info.pNext = NULL;

    VkSemaphore semps_wait[1];
    semps_wait[0] = img_avl_semp;
    VkPipelineStageFlags wait_stages[1];
    wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    sub_info.waitSemaphoreCount = 1;
    sub_info.pWaitSemaphores = &(semps_wait[0]);
    sub_info.pWaitDstStageMask = &(wait_stages[0]);
    sub_info.commandBufferCount = 1;
    sub_info.pCommandBuffers = &(cmd_buf);

    VkSemaphore semps_sig[1];
    semps_sig[0] = rend_fin_semp;

    sub_info.signalSemaphoreCount = 1;
    sub_info.pSignalSemaphores = &(semps_sig[0]);

    vkQueueSubmit(vcw_dev.q_graph, 1, &sub_info, fence);
    //
    // present
    //
    VkPresentInfoKHR pres_info;
    pres_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pres_info.pNext = NULL;
    pres_info.waitSemaphoreCount = 1;
    pres_info.pWaitSemaphores = &(semps_sig[0]);

    VkSwapchainKHR swaps[1];
    swaps[0] = vcw_swap->swap;
    pres_info.swapchainCount = 1;
    pres_info.pSwapchains = &(swaps[0]);
    pres_info.pImageIndices = &img_index;
    pres_info.pResults = NULL;

    result = vkQueuePresentKHR(vcw_dev.q_pres, &pres_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vcw_core.surf->resized) {
        vcw_core.surf->resized = 0;
        recreate_swap(vcw_core, vcw_app);
        return 2;
    } else if (result != VK_SUCCESS) {
        printf("failed to present.\n");
    }

    vcw_app.sync->cur_frame = (cur_frame + 1) % vcw_app.sync->max_frames;
    // printf("current frame: %d\n", vcw_sync->cur_frame);

    clock_t render_end = clock();
    vcw_app.stats->frame_time = (render_end - render_start) / (double) CLOCKS_PER_SEC;

    return 0;
}

void recreate_swap(VCW_VkCoreGroup vcw_core, VCW_App vcw_app) {
    //
    // unpack group arguments
    //
    VCW_PhysicalDevice vcw_phy_dev = *vcw_core.phy_dev;
    VCW_Device vcw_dev = *vcw_core.dev;
    VCW_Surface *vcw_surf = vcw_core.surf;
    VCW_Swapchain *vcw_swap = vcw_core.swap;
    VCW_Renderpass *vcw_rendp = vcw_app.rendp;
    //
    // wait for frame to not be minimized
    //
    printf("\n");
    int width = 0, height = 0;
    glfwGetFramebufferSize(vcw_surf->window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(vcw_surf->window, &width, &height);
        glfwWaitEvents();
    }
    //
    // recreate swapchain
    //
    clock_t start = clock();
    vkDeviceWaitIdle(vcw_dev.dev);

    clean_up_frame_bufs(vcw_dev, *vcw_rendp);
    clean_up_swap(vcw_dev, *vcw_swap);

    update_surface_info(vcw_surf, vcw_phy_dev);

    *vcw_swap = *create_swap(vcw_dev, *vcw_surf, NULL);
    create_frame_bufs(vcw_dev, vcw_rendp, vcw_swap->img_count, vcw_swap->img_views, vcw_swap->extent);

    clock_t end = clock();
    vcw_app.stats->last_swap_recreation_time = (end - start) / (double) CLOCKS_PER_SEC;
}

void clean_up_frame_bufs(VCW_Device vcw_dev, VCW_Renderpass vcw_rendp) {
    for (uint32_t i = 0; i < vcw_rendp.frame_buf_count; i++) {
        vkDestroyFramebuffer(vcw_dev.dev, vcw_rendp.frame_bufs[i], NULL);
        printf("framebuffer %d destroyed.\n", i);
    }
    free(vcw_rendp.frame_bufs);
}

void clean_up_sync(VCW_Device vcw_dev, VCW_Sync vcw_sync) {
    for (uint32_t i = 0; i < vcw_sync.max_frames; i++) {
        vkDestroySemaphore(vcw_dev.dev, vcw_sync.img_avl_semps[i], NULL);
        vkDestroySemaphore(vcw_dev.dev, vcw_sync.rend_fin_semps[i], NULL);
        vkDestroyFence(vcw_dev.dev, vcw_sync.fens[i], NULL);
    }
    free(vcw_sync.img_avl_semps);
    free(vcw_sync.rend_fin_semps);
    free(vcw_sync.fens);
    // free(vcw_sync.img_fens);
}

void destroy_render(VCW_Device vcw_dev, VCW_Pipeline vcw_pipe, VCW_Renderpass vcw_rendp, VCW_Sync vcw_sync) {
    //
    // destroy framebuffers
    //
    clean_up_frame_bufs(vcw_dev, vcw_rendp);
    //
    // destroy sync
    //
    clean_up_sync(vcw_dev, vcw_sync);
    //
    // destroy pipeline and pipeline layout
    //
    vkDestroyPipeline(vcw_dev.dev, vcw_pipe.pipe, NULL);
    printf("graphics pipeline destroyed.\n");
    vkDestroyPipelineLayout(vcw_dev.dev, vcw_pipe.layout, NULL);
    printf("pipeline layout destroyed.\n");
    //
    // destroy render pass
    //
    vkDestroyRenderPass(vcw_dev.dev, vcw_rendp.rendp, NULL);
    printf("render pass destroyed.\n");
}