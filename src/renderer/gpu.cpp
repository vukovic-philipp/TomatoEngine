#include "gpu.hpp"

namespace tomato::gpu {
  bool gpu_init(GPU* g, const char* title, int w, int h) {
    *g = {};
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      SDL_Log("SDL_Init failed: %s", SDL_GetError());
      return false;
    }

    g->window = SDL_CreateWindow("Cube", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!g->window) {
      SDL_Log("Window failed: %s", SDL_GetError());
      return false;
    }

    g->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV,true,nullptr);
    if (!g->device) {
      SDL_Log("Device failed: %s", SDL_GetError());
      return false;
    }
    SDL_Log("Backend: %s", SDL_GetGPUDeviceDriver(g->device));

    g->claimed = SDL_ClaimWindowForGPUDevice(g->device, g->window);
    if (!g->claimed) {
      SDL_Log("Claim failed: %s", SDL_GetError());
      return false;
    }

    return true;
  }

  void gpu_shutdown(GPU* g) {
    if (g->device) SDL_WaitForGPUIdle(g->device);
    if (g->claimed) SDL_ReleaseWindowFromGPUDevice(g->device, g->window);
    if (g->device) SDL_DestroyGPUDevice(g->device);
    if (g->window) SDL_DestroyWindow(g->window);
    *g = {};
  }

  bool depth_ensure(DepthTarget *d, GPU* g, uint32_t w, uint32_t h) {
    if (d->tex && d->w == w && d->h == h) return true;

    if (d->tex) SDL_ReleaseGPUTexture(g->device, d->tex);

    SDL_GPUTextureCreateInfo ti{};
    ti.type                 = SDL_GPU_TEXTURETYPE_2D;
    ti.format               = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    ti.usage                = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    ti.width                = w;
    ti.height               = h;
    ti.layer_count_or_depth = 1;
    ti.num_levels           = 1;
    ti.sample_count         = SDL_GPU_SAMPLECOUNT_1;

    d->tex = SDL_CreateGPUTexture(g->device, &ti);
    d->w   = w;
    d->h   = h;
    if (!d->tex) SDL_Log("Depth texture failed: %s", SDL_GetError());
    return d->tex != nullptr;
  }

  void depth_destroy(DepthTarget *d, GPU* g) {
    if (d->tex) SDL_ReleaseGPUTexture(g->device, d->tex);
    *d = {};
  }

  Frame frame_begin(const GPU* g) {
    Frame f{};
    f.cmd = SDL_AcquireGPUCommandBuffer(g->device);
    SDL_WaitAndAcquireGPUSwapchainTexture(f.cmd, g->window, &f.color, &f.w, &f.h);
    return f;
  }

  void frame_end(Frame* f) {
    SDL_SubmitGPUCommandBuffer(f->cmd);   // always submit, even with color == NULL
    *f = {};
  }

  SDL_GPURenderPass *begin_main_pass(const Frame* f, const DepthTarget* d, SDL_FColor clear) {
    SDL_GPUColorTargetInfo color{};
    color.texture     = f->color;
    color.clear_color = clear;
    color.load_op     = SDL_GPU_LOADOP_CLEAR;
    color.store_op    = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo ds{};
    ds.texture          = d->tex;
    ds.clear_depth      = 1.0f;
    ds.load_op          = SDL_GPU_LOADOP_CLEAR;
    ds.store_op         = SDL_GPU_STOREOP_DONT_CARE;
    ds.stencil_load_op  = SDL_GPU_LOADOP_DONT_CARE;
    ds.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    return SDL_BeginGPURenderPass(f->cmd, &color, 1, &ds);
  }

  SDL_GPUShader* load_shader(gpu::GPU* g, const char* path, SDL_GPUShaderStage stage, uint32_t num_uniform_buffers, uint32_t num_samplers) {
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

    SDL_GPUShader* shader = SDL_CreateGPUShader(g->device, &info);
    SDL_free(code);
    if (!shader) SDL_Log("Shader %s failed: %s", path, SDL_GetError());
    return shader;
  }

}