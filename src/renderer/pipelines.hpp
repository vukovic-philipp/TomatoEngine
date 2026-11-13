#pragma once
#include <glm/vec2.hpp>

#include "gpu.hpp"
#include <glm/vec3.hpp>

namespace tomato::pipelines {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 uv;
    };

    SDL_GPUGraphicsPipeline* create_cube_pipeline(gpu::GPU *g);

}