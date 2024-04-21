#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "render.h"

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    fflush(stdout);

    //load_rdoc();

    VkApplicationInfo app_info = create_app_info();
    VkInstance inst = create_inst(&app_info);
    VCW_PhysicalDevice *phy_dev = get_phy_dev(inst);
    VCW_Device *dev = create_dev(phy_dev);
    VCW_Surface *surf = create_surf(inst, *phy_dev, *dev, (VkExtent2D) {1920, 1080});
    VCW_Swapchain *swap = create_swap(*dev, *surf, VK_NULL_HANDLE);

    VCW_VkCoreGroup vcw_core = {phy_dev, dev, surf, swap};

    VCW_CommandPool cmd_pool = create_cmd_pool(*dev, *swap);
    VCW_Renderpass rendp = create_rendp(*dev, *surf);
    create_frame_bufs(*dev, *swap, &rendp, swap->extent);
    VCW_Sync sync = create_sync(*dev, *swap, 2);

    //
    //
    // buffer creation part
    //
    // create index buffer
    //
    // uint32_t indices[] = {0, 1, 2, 0, 3, 1};
    uint32_t indices[] = {0, 1, 3, 3, 1, 2, 4, 5, 7, 7, 5, 6, 8, 9, 11, 11, 9, 10, 12, 13, 15, 15, 13, 14, 16, 17, 19,
                          19, 17, 18, 20, 21, 23, 23, 21, 22,};
    VCW_Buffer index_buf = create_buffer(*dev, sizeof(indices), sizeof(uint32_t),
                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);

    allocate_memory(*dev, *phy_dev, &index_buf);
    copy_data_to_buf(*dev, &index_buf, &indices[0]);

    printf("index buffer created.\n");
    //
    // create vertex buffer
    //
    /*
    Vertex vertices[] = {{{0.0f,  -0.5f, 1.0f}, {1.0f, 0.0f}},
                         {{0.5f,  0.5f,  1.0f}, {0.0f, 1.0f}},
                         {{-0.5f, 0.5f,  1.0f}, {0.0f, 0.0f}}};
    */

    Vertex vertices[] = {
            {{-0.5f, 0.5f,  -0.5f}, {0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
            {{0.5f,  -0.5f, -0.5f}, {1.0f, 1.0f}},
            {{0.5f,  0.5f,  -0.5f}, {1.0f, 0.0f}},
            {{-0.5f, 0.5f,  0.5f},  {0.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.5f},  {0.0f, 1.0f}},
            {{0.5f,  -0.5f, 0.5f},  {1.0f, 1.0f}},
            {{0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
            {{0.5f,  0.5f,  -0.5f}, {0.0f, 0.0f}},
            {{0.5f,  -0.5f, -0.5f}, {0.0f, 1.0f}},
            {{0.5f,  -0.5f, 0.5f},  {1.0f, 1.0f}},
            {{0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
            {{-0.5f, 0.5f,  -0.5f}, {0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
            {{-0.5f, -0.5f, 0.5f},  {1.0f, 1.0f}},
            {{-0.5f, 0.5f,  0.5f},  {1.0f, 0.0f}},
            {{-0.5f, 0.5f,  0.5f},  {0.0f, 0.0f}},
            {{-0.5f, 0.5f,  -0.5f}, {0.0f, 1.0f}},
            {{0.5f,  0.5f,  -0.5f}, {1.0f, 1.0f}},
            {{0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
            {{-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f}},
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},
            {{0.5f,  -0.5f, -0.5f}, {1.0f, 1.0f}},
            {{0.5f,  -0.5f, 0.5f},  {1.0f, 0.0f}}
    };


    VCW_Buffer vert_buf = create_buffer(*dev, sizeof(vertices), sizeof(struct Vertex),
                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);

    allocate_memory(*dev, *phy_dev, &vert_buf);
    copy_data_to_buf(*dev, &vert_buf, &vertices[0]);

    printf("vertex buffer created.\n");
    //
    // create uniform buffer
    //
    VCW_Uniform unif;
    glm_mat4_zero(unif.data);

    VCW_Buffer *unif_bufs = malloc(swap->img_count * sizeof(VCW_Buffer));
    for (uint32_t i = 0; i < sync.max_frames; i++) {
        unif_bufs[i] = create_buffer(*dev, sizeof(unif), sizeof(VCW_Uniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_SHARING_MODE_EXCLUSIVE);

        allocate_memory(*dev, *phy_dev, &unif_bufs[i]);
        map_mem(*dev, &unif_bufs[i]);
    }
    printf("uniform buffers created.\n");
    //
    //
    // descriptor creation part
    //
    // create descriptor pool
    //
    VCW_DescriptorPool desc_group = create_vcw_desc();
    add_desc_set_layout(*dev, &desc_group, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, 1);
    add_desc_set_layout(*dev, &desc_group, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL, 1);

    VkDescriptorPoolSize unif_pool_size;
    unif_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // number of individual descriptors of that type
    unif_pool_size.descriptorCount = 2;
    VkDescriptorPoolSize pool_sizes[] = {unif_pool_size};
    init_desc_pool(*dev, &desc_group, &pool_sizes[0], 1);
    //
    // write uniform descriptor
    //
    for (uint32_t i = 0; i < sync.max_frames; i++) {
        write_buffer_desc(*dev, &desc_group, &unif_bufs[i], i, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    VCW_Pipeline pipe = create_pipe(*dev, rendp, desc_group, swap->extent);

    VCW_PushConstant push_const;
    glm_mat4_zero(push_const.view);
    glm_vec2_zero(push_const.res);
    push_const.time = 0;

    VCW_App vcw_app;
    vcw_app.cmd = &cmd_pool;
    vcw_app.rendp = &rendp;
    vcw_app.pipe = &pipe;
    vcw_app.desc = &desc_group;
    vcw_app.sync = &sync;
    vcw_app.vert_buf = &vert_buf;
    vcw_app.index_buf = &index_buf;
    vcw_app.index_count = 36;
    vcw_app.cpu_unif = &unif;
    vcw_app.cpu_push_const = &push_const;
    vcw_app.unif_bufs = unif_bufs;
    vcw_app.unif_buf_count = sync.max_frames;
    vcw_app.frame_count = 0;

    printf("\n");
    while (!glfwWindowShouldClose(surf->window)) {
        glfwPollEvents();
        //
        // submit
        //
        render(vcw_core, vcw_app);
        vcw_app.frame_count += 1;
        vcw_app.cpu_push_const->time += 1;
        glm_vec2_copy((vec2) {swap->extent.width, swap->extent.height}, vcw_app.cpu_push_const->res);

        float sensitivity = 0.1f;
        float yaw = glm_rad((float)(surf->cursor_position[0]) * sensitivity);
        float pitch = glm_rad((float)(-surf->cursor_position[1]) * sensitivity);

        vec3 cam_front;
        cam_front[0] = cosf(yaw) * cosf(pitch);
        cam_front[1] = sinf(pitch);
        cam_front[2] = sinf(yaw) * cosf(pitch);
        glm_vec3_normalize(cam_front);

        vec3 cam_pos = {0.0f, 0.0f, -1.0f};
        glm_vec3_add(cam_pos, cam_front, cam_front);
        mat4 view;
        glm_translate(view, (vec3) {0.0f, 0.0f, 0.0f});
        mat4 projection;
        glm_perspective(push_const.res[0] / push_const.res[1], glm_rad(45.0f), 0.1f, 200.0f, projection);
        mat4 look_at;
        vec4 cam_up = {0.0f, 1.0f, 0.0f, 0.0f};
        glm_lookat(cam_pos, cam_front, (vec3){0.0f, 1.0f, 0.0f}, look_at);

        glm_mat4_mul(projection, look_at, push_const.view);
        //glm_mat4_mul(push_const.view, view, push_const.view);

        /*

        float angle = glm_rad((float) (push_const.time % 360));

        vec3 cameraPosition = {0.0f, 0.0f, -1.0f}; // Example camera position
        vec3 targetPosition;
        glm_vec3_add(cameraPosition, cam_front, targetPosition);

        mat4 viewMatrix;
        glm_lookat(cameraPosition, targetPosition, (vec3){0.0f, 1.0f, 0.0f}, viewMatrix);

        mat4 rotationMatrix;
        glm_rotate_make(rotationMatrix, angle, (vec3) {0.0f, 0.0f, 1.0f});

        mat4 projectionMatrix;
        glm_perspective(push_const.res[0] / push_const.res[1], 1.0f, 0.1f, 100.0f, projectionMatrix);

        //mat4 viewMatrix;
        //glm_mat4_mul(projectionMatrix, rotationMatrix, viewMatrix);

        glm_mat4_copy(viewMatrix, push_const.view);
        glm_mat4_mul(projectionMatrix, viewMatrix, push_const.view);
        */
    }
    vkDeviceWaitIdle(dev->dev);
    printf("command buffers finished.\n");

    destroy_render(*dev, pipe, rendp, sync);
    destroy_vk_core(inst, *dev, *swap, *surf, cmd_pool);

    return 0;
}