#pragma once
#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gpu.hpp"

namespace tomato::renderer {
    struct ObjectUBO {
        glm::mat4 mvp,
        model,
        normal_mat;
    };

    struct LightUBO {
        glm::vec4 dir_to_light,
        color,
        ambient,
        cam_pos;
    };

    int renderer(const char* path);
}