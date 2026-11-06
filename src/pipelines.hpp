#pragma once
#include "gpu.hpp"
#include <glm/vec3.hpp>

namespace tomato::pipelines {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
    };

    SDL_GPUGraphicsPipeline* create_cube_pipeline(gpu::GPU *g);

}