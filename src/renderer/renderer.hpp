#pragma once
#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gpu.hpp"

namespace tomato::renderer {
    int renderer(const char* path);
}