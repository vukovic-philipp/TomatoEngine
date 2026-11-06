#include "main.hpp"
#include "mesh.hpp"

namespace tomato::renderer {
    static_assert(sizeof(pipelines::Vertex) == 24, "Vertex must be tightly packed");

    static const pipelines::Vertex cube_vertices[8] = {
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // 0 red
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},  // 1 green
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},  // 2 blue
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},  // 3 yellow
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},  // 4 magenta
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},  // 5 cyan
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},  // 6 white
        {{-0.5f,  0.5f,  0.5f}, {0.4f, 0.4f, 0.4f}},  // 7 gray
    };

    static const uint32_t cube_indices[36] = {
        4, 5, 6,  4, 6, 7,   // front  (+Z)
        0, 3, 2,  0, 2, 1,   // back   (-Z)
        1, 2, 6,  1, 6, 5,   // right  (+X)
        0, 4, 7,  0, 7, 3,   // left   (-X)
        3, 7, 6,  3, 6, 2,   // top    (+Y)
        0, 1, 5,  0, 5, 4,   // bottom (-Y)
    };


    int main(int, char **)
    {
        gpu::GPU gpu {};
        if (!gpu_init(&gpu, "Cube", 1280, 720)) { gpu_shutdown(&gpu); return 1; }

        SDL_GPUGraphicsPipeline *pipeline = pipelines::create_cube_pipeline(&gpu);
        mesh::Mesh cube {};
        if (!pipeline || !mesh_create(&cube, &gpu, cube_vertices, 8, cube_indices, 36)) {
            gpu_shutdown(&gpu);
            return 1;
        }
        gpu::DepthTarget depth{};

        // Scene: struct-of-arrays. Entity i is index i across all arrays.
        constexpr int N = 5;
        const glm::vec3 positions[N]  = { {-4,0,0}, {-2,0,0}, {0,0,0}, {2,0,0}, {4,0,0} };
        const float     spin_speed[N] = { 0.2f, 0.5f, 1.0f, 1.5f, 2.5f };
        const glm::vec3 spin_axis(0.5f, 1.0f, 0.0f);

        bool running = true;
        while (running) {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                if (e.type == SDL_EVENT_QUIT) running = false;

            gpu::Frame f = frame_begin(&gpu);
            if (f.color && depth_ensure(&depth, &gpu, f.w, f.h)) {
                const float t      = SDL_GetTicks() / 1000.0f;
                const float aspect = static_cast<float>(f.w) / static_cast<float>(f.h);

                const glm::mat4 view = glm::lookAt(glm::vec3(0, 2, 8), glm::vec3(0),
                                                   glm::vec3(0, 1, 0));
                const glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
                const glm::mat4 vp   = proj * view;   // same for every object this frame

                SDL_GPURenderPass *pass =
                    begin_main_pass(&f, &depth, SDL_FColor{0.1f, 0.1f, 0.15f, 1.0f});
                SDL_BindGPUGraphicsPipeline(pass, pipeline);

                for (int i = 0; i < N; ++i) {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), positions[i])
                                    * glm::rotate(glm::mat4(1.0f), t * spin_speed[i], spin_axis);
                    glm::mat4 mvp = vp * model;
                    SDL_PushGPUVertexUniformData(f.cmd, 0, &mvp, sizeof(mvp));
                    mesh_draw(pass, &cube);
                }

                SDL_EndGPURenderPass(pass);
            }
            frame_end(&f);
        }

        SDL_WaitForGPUIdle(gpu.device);
        depth_destroy(&depth, &gpu);
        mesh_destroy(&cube, &gpu);
        SDL_ReleaseGPUGraphicsPipeline(gpu.device, pipeline);
        gpu_shutdown(&gpu);
        return 0;
    }
}

int main(int argc, char **argv) {
    return tomato::renderer::main(argc, argv);
}