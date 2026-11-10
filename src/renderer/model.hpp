#pragma once
#include "pipelines.hpp"

namespace tomato::model {
    struct MeshData {
        pipelines::Vertex* verts;
        uint32_t vcount;
        uint32_t* idx;
        uint32_t icount;
    };

    bool model_load(MeshData* out, const char* path);
    void model_free(MeshData* mesh);
}