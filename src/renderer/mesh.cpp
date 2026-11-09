#include "mesh.hpp"
#include <cstring>

namespace tomato::mesh {

    bool mesh_create(Mesh* m, gpu::GPU* g, const pipelines::Vertex *verts, uint32_t vcount, const uint32_t *idx, uint32_t icount) {
    *m = {};
    const uint32_t vbytes = vcount * sizeof(pipelines::Vertex);
    const uint32_t ibytes = icount * sizeof(uint32_t);

    SDL_GPUBufferCreateInfo vbi{};
    vbi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbi.size  = vbytes;
    m->vertex_buffer = SDL_CreateGPUBuffer(g->device, &vbi);

    SDL_GPUBufferCreateInfo ibi{};
    ibi.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibi.size  = ibytes;
    m->index_buffer = SDL_CreateGPUBuffer(g->device, &ibi);

    SDL_GPUTransferBufferCreateInfo tbi{};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size  = vbytes + ibytes;
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(g->device, &tbi);

    if (!m->vertex_buffer || !m->index_buffer || !transfer) {
        SDL_Log("Mesh create failed: %s", SDL_GetError());
        if (transfer) SDL_ReleaseGPUTransferBuffer(g->device, transfer);
        mesh_destroy(m, g);
        return false;
    }

    auto* mapped = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(g->device, transfer, false));
    std::memcpy(mapped,          verts, vbytes);
    std::memcpy(mapped + vbytes, idx,   ibytes);
    SDL_UnmapGPUTransferBuffer(g->device, transfer);

    SDL_GPUCommandBuffer *cmd  = SDL_AcquireGPUCommandBuffer(g->device);
    SDL_GPUCopyPass      *copy = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src{ transfer, 0 };
    SDL_GPUBufferRegion           dst{ m->vertex_buffer, 0, vbytes };
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);

    src.offset = vbytes;
    dst        = { m->index_buffer, 0, ibytes };
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);

    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(g->device, transfer);

    m->index_count = icount;
    return true;
}

    void mesh_destroy(Mesh* m, gpu::GPU* g) {
    if (m->vertex_buffer) SDL_ReleaseGPUBuffer(g->device, m->vertex_buffer);
    if (m->index_buffer)  SDL_ReleaseGPUBuffer(g->device, m->index_buffer);
    *m = {};
}

    void mesh_draw(SDL_GPURenderPass* pass, const Mesh* m) {
        SDL_GPUBufferBinding vb{ m->vertex_buffer, 0 };
        SDL_GPUBufferBinding ib{ m->index_buffer, 0 };
        SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
        SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(pass, m->index_count, 1, 0, 0, 0);
    }
}
