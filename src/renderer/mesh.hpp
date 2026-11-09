#pragma once
#include <SDL3/SDL.h>
#include "gpu.hpp"
#include "pipelines.hpp"

namespace tomato::mesh {
    struct Mesh {
        SDL_GPUBuffer *vertex_buffer;
        SDL_GPUBuffer *index_buffer;
        uint32_t         index_count;
    };

    bool mesh_create(Mesh* m, gpu::GPU* g, const pipelines::Vertex *verts, uint32_t vcount, const uint32_t *idx, uint32_t icount);

    void mesh_destroy(Mesh* m, gpu::GPU* g);

    void mesh_draw(SDL_GPURenderPass* pass, const Mesh* m);
}
