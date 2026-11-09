#include "texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <cstring>

#include "third_party/stb_image.h"

namespace tomato::texture {
    bool texture_load(Texture* t, gpu::GPU* g, const char* path) {
        *t = {};

        int w, h, channels;
        stbi_uc* px = stbi_load(path, &w, &h, &channels, 4);
        if (!px) {
            SDL_Log("Could not load %s, %s", path, stbi_failure_reason());
            return false;
        }
        const uint32_t bytes = static_cast<uint32_t>(w) * static_cast<uint32_t>(h) * 4;
        const uint32_t levels = std::bit_width(static_cast<uint32_t>(std::max(w, h)));

        SDL_GPUTextureCreateInfo ti {};
        ti.type = SDL_GPU_TEXTURETYPE_2D;
        ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        ti.height = h;
        ti.width = w;
        ti.layer_count_or_depth = 1;
        ti.num_levels = levels;
        ti.sample_count = SDL_GPU_SAMPLECOUNT_1;
        t->tex = SDL_CreateGPUTexture(g->device, &ti);

        SDL_GPUTransferBufferCreateInfo tbi {};
        tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbi.size = bytes;
        SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(g->device, &tbi);

        SDL_GPUSamplerCreateInfo si {};
        si.min_filter = SDL_GPU_FILTER_LINEAR;
        si.mag_filter = SDL_GPU_FILTER_LINEAR;
        si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        si.max_anisotropy = 4.0f;
        si.enable_anisotropy = true;
        si.max_lod = 1000.0f;
        t->sampler = SDL_CreateGPUSampler(g->device, &si);

        if (!t->tex || !transfer || !t->sampler) {
            SDL_Log("Texture create failed: %s", SDL_GetError());
            if (transfer) SDL_ReleaseGPUTransferBuffer(g->device, transfer);
            stbi_image_free(px);
            texture_destroy(t, g);
            return false;
        }

        void* mapped = SDL_MapGPUTransferBuffer(g->device, transfer, false);
            std::memcpy(mapped, px, bytes);
            SDL_UnmapGPUTransferBuffer(g->device, transfer);
            stbi_image_free(px);

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(g->device);
        SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo src {};
        src.transfer_buffer = transfer;
        src.pixels_per_row = w;
        src.rows_per_layer = h;

        SDL_GPUTextureRegion dst {};
        dst.texture = t->tex;
        dst.w = w;
        dst.h = h;
        dst.d = 1;

        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);

        if (levels > 1) SDL_GenerateMipmapsForGPUTexture(cmd, t->tex);
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(g->device, transfer);

        t->w = w;
        t->h = h;
        return true;
    }

    void texture_destroy(Texture* t, gpu::GPU* g) {
        if (t->sampler) SDL_ReleaseGPUSampler(g->device, t->sampler);
        if (t->tex) SDL_ReleaseGPUTexture(g->device, t->tex);
        *t = {};
    }

    void texture_bind(SDL_GPURenderPass* pass, const Texture* t, uint32_t slot) {
        SDL_GPUTextureSamplerBinding bind;
        bind.sampler = t->sampler;
        bind.texture = t->tex;
        SDL_BindGPUFragmentSamplers(pass, slot, &bind, 1);
    }
}