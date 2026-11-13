#pragma once

#include <vector>

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "mesh.hpp"
#include "model.hpp"
#include "texture.hpp"
#include "gpu.hpp"

namespace tomato::renderer {
    struct RegisteredModel {
        mesh::Mesh mesh;
        texture::Texture tex;
    };

    struct Renderable {
        RegisteredModel* model;
        glm::vec3 position{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    };

    struct Renderer {
        gpu::GPU gpu;
        SDL_GPUGraphicsPipeline* pipeline;
        gpu::DepthTarget depth;
        std::vector<RegisteredModel*> Models;
        std::vector<Renderable*> renderables;
    };

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

    bool renderer_create(Renderer* r, const char* name);
    bool renderer_register_model(Renderer* r, RegisteredModel* m, const char* model_path, const char* tex_path = "assets/LordKorea.png");
    void renderer_add_renderable(Renderer* r, Renderable* renderable);
    void renderer_frame_tick(Renderer* r);
}