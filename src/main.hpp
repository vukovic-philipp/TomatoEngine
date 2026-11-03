#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

static SDL_GPUShader* LoadShader(
    SDL_GPUDevice* device,
    const char* path,
    SDL_GPUShaderStage stage,
    uint32_t num_uniform_buffers = 0,
    uint32_t num_samplers = 0) {
    size_t size = 0;
    void* code = SDL_LoadFile(path, &size);
    if (!code) {
        SDL_Log("could not Load %s: %s", path, SDL_GetError());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo info {};
    info.code                   = static_cast<const uint8_t*>(code);
    info.code_size              = size;
    info.entrypoint             = "main";
    info.format                 = SDL_GPU_SHADERFORMAT_SPIRV;
    info.stage                  = stage;
    info.num_uniform_buffers    = num_uniform_buffers;
    info.num_samplers           = num_samplers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &info);
    SDL_free(code);
    if (!shader) SDL_Log("Shader %s failed: %s", path, SDL_GetError());
    return shader;
}

static SDL_GPUTexture* CreateDepthTexture(SDL_GPUDevice* device, uint32_t w, uint32_t h) {
    SDL_GPUTextureCreateInfo ti {};
    ti.type                 = SDL_GPU_TEXTURETYPE_2D;
    ti.format               = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    ti.usage                = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    ti.width                = w;
    ti.height               = h;
    ti.layer_count_or_depth = 1;
    ti.num_levels           = 1;
    ti.sample_count         = SDL_GPU_SAMPLECOUNT_1;

    SDL_GPUTexture* tex = SDL_CreateGPUTexture(device, &ti);
    if (!tex) SDL_Log("Depth Texture failed: %s", SDL_GetError());
    return tex;
}