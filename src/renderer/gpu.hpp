#pragma once
#include <SDL3/SDL.h>

namespace tomato::gpu {
    struct GPU {
        SDL_Window* window;
        SDL_GPUDevice* device;
        bool claimed;
    };

    struct DepthTarget {
        SDL_GPUTexture *tex;
        uint32_t w, h;
    };

    struct Frame {
        SDL_GPUCommandBuffer *cmd;
        SDL_GPUTexture       *color;
        Uint32                w, h;
    };

    bool gpu_init(GPU* g, const char* title, int w, int h);

    void gpu_shutdown(GPU* g);

    bool depth_ensure(DepthTarget *d, GPU* g, uint32_t w, uint32_t h);

    void depth_destroy(DepthTarget *d, GPU* g);

    Frame frame_begin(const GPU* g);

    void frame_end(Frame* f);

    SDL_GPURenderPass *begin_main_pass(const Frame* f, const DepthTarget* d, SDL_FColor clear);

    SDL_GPUShader* load_shader(gpu::GPU* g, const char* path, SDL_GPUShaderStage stage, uint32_t num_uniform_buffers = 0, uint32_t num_samplers = 0);

}