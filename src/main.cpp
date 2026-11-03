#include "main.hpp"
#include <cstring>

static_assert(sizeof(Vertex) == 24, "Vertex must be tightly packed");

static const Vertex cube_vertices[8] = {
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // 0 red
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},  // 1 green
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},  // 2 blue
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},  // 3 yellow
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},  // 4 magenta
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},  // 5 cyan
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},  // 6 white
    {{-0.5f,  0.5f,  0.5f}, {0.4f, 0.4f, 0.4f}},  // 7 gray
};

static const uint16_t cube_indices[36] = {
    4, 5, 6,  4, 6, 7,   // front  (+Z)
    0, 3, 2,  0, 2, 1,   // back   (-Z)
    1, 2, 6,  1, 6, 5,   // right  (+X)
    0, 4, 7,  0, 7, 3,   // left   (-X)
    3, 7, 6,  3, 6, 2,   // top    (+Y)
    0, 1, 5,  0, 5, 4,   // bottom (-Y)
};

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Cube", 1280, 720, 0);
    if (!window) {
        SDL_Log("Window failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV,true,nullptr);
    if (!device) {
        SDL_Log("Device failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Log("Backend: %s", SDL_GetGPUDeviceDriver(device));

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("Claim failed: %s", SDL_GetError());
    return 1;
    }



    SDL_GPUShader* vs = LoadShader(device, "shaders/cube.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1);
    SDL_GPUShader* fs = LoadShader(device, "shaders/tri.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (!vs||!fs) return 1;

    SDL_GPUColorTargetDescription color_desc {};
    color_desc.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUVertexBufferDescription vb_desc {};
    vb_desc.slot = 0;
    vb_desc.pitch = sizeof(Vertex);
    vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[2] {};
    attrs[0].location = 0;
    attrs[0].buffer_slot = 0;
    attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[0].offset = offsetof(Vertex, pos);
    attrs[1].location = 1;
    attrs[1].buffer_slot = 0;
    attrs[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[1].offset = offsetof(Vertex, color);

    SDL_GPUGraphicsPipelineCreateInfo pi {};
    pi.vertex_shader = vs;
    pi.fragment_shader = fs;
    pi.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pi.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pi.target_info.color_target_descriptions = &color_desc;
    pi.target_info.num_color_targets = 1;

    pi.vertex_input_state.vertex_buffer_descriptions = &vb_desc;
    pi.vertex_input_state.num_vertex_buffers = 1;
    pi.vertex_input_state.vertex_attributes = attrs;
    pi.vertex_input_state.num_vertex_attributes = 2;

    pi.depth_stencil_state.enable_depth_test    = true;
    pi.depth_stencil_state.enable_depth_write   = true;
    pi.depth_stencil_state.compare_op           = SDL_GPU_COMPAREOP_LESS;

    pi.target_info.has_depth_stencil_target = true;
    pi.target_info.depth_stencil_format     = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    pi.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pi.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pi);
    if (!pipeline) {
        SDL_Log("Pipeline failed: %s", SDL_GetError());
        return 1;
    }

    SDL_ReleaseGPUShader(device, vs);
    SDL_ReleaseGPUShader(device, fs);



    const uint32_t vertex_bytes = sizeof(cube_vertices);
    const uint32_t index_bytes = sizeof(cube_indices);

    SDL_GPUBufferCreateInfo vbi {};
    vbi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbi.size = vertex_bytes;
    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(device, &vbi);

    SDL_GPUBufferCreateInfo ibi {};
    ibi.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibi.size = index_bytes;
    SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(device, &ibi);

    SDL_GPUTransferBufferCreateInfo tbi {};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size  = vertex_bytes + index_bytes;
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &tbi);

    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, cube_vertices, vertex_bytes);
    std::memcpy(mapped + vertex_bytes, cube_indices, index_bytes);
    SDL_UnmapGPUTransferBuffer(device, transfer);


    SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(upload_cmd);

    SDL_GPUTransferBufferLocation src {};
    SDL_GPUBufferRegion dst {};

    src.transfer_buffer = transfer;
    src.offset = 0;
    dst.buffer = vertex_buffer;
    dst.offset = 0;
    dst.size = vertex_bytes;
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);

    src.offset = vertex_bytes;
    dst.buffer = index_buffer;
    dst.size = index_bytes;
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);

    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(upload_cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);


    SDL_GPUTexture* depth_tex = nullptr;
    uint32_t depth_w = 0, depth_h = 0;


    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
        }

        // 1. Get a command buffer to record into
        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);

        // 2. Get this frame's swapchain image
        SDL_GPUTexture* swapchain_tex = nullptr;
        uint32_t sw = 0, sh = 0;
        SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain_tex, &sw, &sh);

        // The Texture can be NULL. Still must submit.
        if (swapchain_tex) {
            if (!depth_tex || sw != depth_w || sh != depth_h) {
                if (depth_tex) SDL_ReleaseGPUTexture(device, depth_tex);
                depth_tex = CreateDepthTexture(device, sw, sh);
                depth_w = sw;
                depth_h = sh;
            }

            SDL_GPUColorTargetInfo color {};
            color.texture = swapchain_tex;
            color.clear_color = SDL_FColor{0.1f,0.1f,0.15f,0.1f};
            color.load_op = SDL_GPU_LOADOP_CLEAR; // clear at start of pass
            color.store_op = SDL_GPU_STOREOP_STORE; // keep result at end

            float t = SDL_GetTicks() / 1000.0f;
            float aspect = static_cast<float>(sw) / static_cast<float>(sh);

            glm::mat4 model = glm::rotate(glm::mat4(1.0f), t, glm::vec3(0.5f, 1.0f, 0.0f));
            glm::mat4 view  = glm::lookAt(
                glm::vec3(0.0f, 1.5f,3.0f),
                glm::vec3(0.0f,0.0f,0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

            glm::mat4 mvp = proj * view * model;
            SDL_PushGPUVertexUniformData(cmd, 0, &mvp, sizeof(mvp));

            SDL_GPUDepthStencilTargetInfo ds {};
            ds.texture = depth_tex;
            ds.clear_depth = 1.0f;
            ds.load_op = SDL_GPU_LOADOP_CLEAR;
            ds.store_op = SDL_GPU_STOREOP_DONT_CARE;
            ds.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
            ds.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &color, 1, &ds);

            SDL_BindGPUGraphicsPipeline(pass, pipeline);

            SDL_GPUBufferBinding vb_bind { vertex_buffer, 0 };
            SDL_BindGPUVertexBuffers(pass, 0, &vb_bind, 1);

            SDL_GPUBufferBinding ib_bind { index_buffer, 0 };
            SDL_BindGPUIndexBuffer(pass, &ib_bind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);

            SDL_EndGPURenderPass(pass);
        }

        // 3. Submit: GPU executes it, image gets presented
        SDL_SubmitGPUCommandBuffer(cmd);
    }

    if (depth_tex) SDL_ReleaseGPUTexture(device, depth_tex);
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
    SDL_ReleaseGPUBuffer(device, index_buffer);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

