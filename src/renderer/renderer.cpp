#include "renderer.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "texture.hpp"

namespace tomato::renderer {
    static_assert(sizeof(pipelines::Vertex) == 32, "Vertex must be tightly packed");

    struct Face { glm::vec3 o, u, v; };
    static const Face faces[6] = {
        {{-.5f,-.5f, .5f}, { 1,0,0}, {0,1,0}},   // +Z
        {{ .5f,-.5f,-.5f}, {-1,0,0}, {0,1,0}},   // -Z
        {{ .5f,-.5f, .5f}, {0,0,-1}, {0,1,0}},   // +X
        {{-.5f,-.5f,-.5f}, {0,0, 1}, {0,1,0}},   // -X
        {{-.5f, .5f, .5f}, { 1,0,0}, {0,0,-1}},  // +Y
        {{-.5f,-.5f,-.5f}, { 1,0,0}, {0,0, 1}},  // -Y
    };

    static void build_cube(pipelines::Vertex out_v[24], uint32_t out_i[36]) {
        const glm::vec3 white(1.0f);
        for (int f = 0; f < 6; ++f) {
            const Face& fc = faces[f];
            pipelines::Vertex* v = out_v + f * 4;
            v[0] = { fc.o,                 white, {0, 1} };
            v[1] = { fc.o + fc.u,          white, {1, 1} };
            v[2] = { fc.o + fc.u + fc.v,   white, {1, 0} };
            v[3] = { fc.o + fc.v,          white, {0, 0} };
            uint32_t b = f * 4;
            uint32_t* i = out_i + f * 6;
            i[0] = b; i[1] = b+1; i[2] = b+2;
            i[3] = b; i[4] = b+2; i[5] = b+3;
        }
    }


    int renderer() {
        gpu::GPU gpu {};
        if (!gpu_init(&gpu, "Cube", 1280, 720)) { gpu_shutdown(&gpu); return 1; }

        SDL_GPUGraphicsPipeline *pipeline = pipelines::create_cube_pipeline(&gpu);
        /*pipelines::Vertex verts[24]; uint32_t idx[36];
        build_cube(verts, idx);

        mesh::Mesh cube {};
        texture::Texture tex {};
        if (!pipeline
            || !mesh_create(&cube, &gpu, verts, 24, idx, 36)
            || !texture_load(&tex, &gpu, "assets/LordKorea.png")) {
            if (tex.tex) texture::texture_destroy(&tex, &gpu);
            if (!pipeline) gpu::gpu_shutdown(&gpu);
            return 1;
        }*/

        model::MeshData md {};
        if (!model::model_load(&md, "assets/BoxTextured.glb")) {
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
