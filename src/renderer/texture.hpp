#pragma once
#include <SDL3/SDL.h>
#include "gpu.hpp"
namespace tomato::texture {
    struct Texture {
        SDL_GPUTexture* tex;
        SDL_GPUSampler* sampler;
        uint32_t w, h;
    };

    bool texture_load(Texture* t, gpu::GPU* g, const char* path);
    void texture_destroy(Texture* t, gpu::GPU* g);
    void texture_bind(SDL_GPURenderPass* pass, const Texture* t, uint32_t slot);

}