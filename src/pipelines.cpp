#include "pipelines.hpp"

#include "main.hpp"

namespace tomato::pipelines {

    SDL_GPUGraphicsPipeline* create_cube_pipeline(gpu::GPU* g) {
    SDL_GPUShader *vs = gpu::load_shader(g, "shaders/cube.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX, 1);
    SDL_GPUShader *fs = gpu::load_shader(g, "shaders/tri.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT);
    if (!vs || !fs) {
        if (vs) SDL_ReleaseGPUShader(g->device, vs);
        if (fs) SDL_ReleaseGPUShader(g->device, fs);
        return nullptr;
    }

    SDL_GPUVertexBufferDescription vb_desc{};
    vb_desc.slot       = 0;
    vb_desc.pitch      = sizeof(Vertex);
    vb_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute attrs[2]{};
    attrs[0].location    = 0;
    attrs[0].buffer_slot = 0;
    attrs[0].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[0].offset      = offsetof(Vertex, pos);
    attrs[1].location    = 1;
    attrs[1].buffer_slot = 0;
    attrs[1].format      = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attrs[1].offset      = offsetof(Vertex, color);

    SDL_GPUColorTargetDescription color_desc{};
    color_desc.format = SDL_GetGPUSwapchainTextureFormat(g->device, g->window);

    SDL_GPUGraphicsPipelineCreateInfo pi{};
    pi.vertex_shader   = vs;
    pi.fragment_shader = fs;
    pi.primitive_type  = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    pi.vertex_input_state.vertex_buffer_descriptions = &vb_desc;
    pi.vertex_input_state.num_vertex_buffers         = 1;
    pi.vertex_input_state.vertex_attributes          = attrs;
    pi.vertex_input_state.num_vertex_attributes      = 2;

    pi.rasterizer_state.fill_mode  = SDL_GPU_FILLMODE_FILL;
    pi.rasterizer_state.cull_mode  = SDL_GPU_CULLMODE_BACK;
    pi.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    pi.depth_stencil_state.enable_depth_test  = true;
    pi.depth_stencil_state.enable_depth_write = true;
    pi.depth_stencil_state.compare_op         = SDL_GPU_COMPAREOP_LESS;

    pi.target_info.color_target_descriptions = &color_desc;
    pi.target_info.num_color_targets         = 1;
    pi.target_info.has_depth_stencil_target  = true;
    pi.target_info.depth_stencil_format      = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(g->device, &pi);
    if (!pipeline) SDL_Log("Pipeline failed: %s", SDL_GetError());

    SDL_ReleaseGPUShader(g->device, vs);
    SDL_ReleaseGPUShader(g->device, fs);
    return pipeline;
    }

}
