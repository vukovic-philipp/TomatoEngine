#include "renderer.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "texture.hpp"

namespace tomato::renderer {
    static_assert(sizeof(pipelines::Vertex) == 32, "Vertex must be tightly packed");

    int renderer(const char* path) {
        gpu::GPU gpu {};
        if (!gpu_init(&gpu, "TomatoEngine v0.1.0", 1280, 720)) { gpu_shutdown(&gpu); return 1; }

        SDL_GPUGraphicsPipeline *pipeline = pipelines::create_cube_pipeline(&gpu);

        model::MeshData md {};
        if (!model::model_load(&md, path)) {
            gpu::gpu_shutdown(&gpu);
            return 1;
        }
        mesh::Mesh cube {};
        texture::Texture tex {};
        bool ok = mesh_create(&cube, &gpu, md.verts, md.vcount, md.idx, md.icount);
        model::model_free(&md);

        if (!pipeline
            || !ok
            || !texture_load(&tex, &gpu, "assets/LordKorea.png")) {
            if (tex.tex) texture::texture_destroy(&tex, &gpu);
            if (!pipeline) gpu::gpu_shutdown(&gpu);
            gpu_shutdown(&gpu);
            return 1;
        }


        gpu::DepthTarget depth{};

        // Scene: struct-of-arrays. Entity i is index i across all arrays.
        constexpr int N = 5;
        glm::vec3 positions[N]  = { {-4,0,0}, {-2,0,0}, {0,0,0}, {2,0,0}, {4,0,0} };
        const float     spin_speed[N] = { 0.2f, 0.5f, 1.0f, 1.5f, 2.5f };
        const glm::vec3 spin_axis(0.5f, 1.0f, 0.0f);

        bool running = true;
        while (running) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) running = false;
                if (e.type == SDL_EVENT_KEY_DOWN) {
                    switch (e.key.key) {
                        case(SDLK_D): positions[2].x += 0.1f; break;
                        case(SDLK_A): positions[2].x -= 0.1f; break;
                        case(SDLK_W): positions[2].y += 0.1f; break;
                        case(SDLK_S): positions[2].y -= 0.1f; break;
                        case(SDLK_E): positions[2].z += 0.1f; break;
                        case(SDLK_Q): positions[2].z -= 0.1f; break;
                    }
                }
            }

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
                texture::texture_bind(pass, &tex, 0);

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
        texture::texture_destroy(&tex, &gpu);
        depth_destroy(&depth, &gpu);
        mesh_destroy(&cube, &gpu);
        SDL_ReleaseGPUGraphicsPipeline(gpu.device, pipeline);
        gpu_shutdown(&gpu);
        return 0;
    }
}