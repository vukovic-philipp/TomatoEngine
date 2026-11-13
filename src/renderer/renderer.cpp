#include "renderer.hpp"

namespace tomato::renderer {
    static_assert(sizeof(pipelines::Vertex) == 44, "Vertex must be tightly packed");

    bool renderer_create(Renderer* r, const char* name) {
        if (!gpu::gpu_init(&r->gpu, name, 1280, 720)) { gpu_shutdown(&r->gpu); return false; }

        r->pipeline = pipelines::create_cube_pipeline(&r->gpu);
        if (!r->pipeline) {
            gpu::gpu_shutdown(&r->gpu);
            return false;
        }
        return true;
    }

    bool renderer_register_model(Renderer* r, RegisteredModel* m, const char* model_path, const char* tex_path) {
        if (!tex_path) tex_path = "assets/LordKorea.png";
        *m = {};

        model::MeshData md {};
        if (!model::model_load(&md, model_path)) {
            gpu::gpu_shutdown(&r->gpu);
            return false;
        }
        bool ok = mesh_create(&m->mesh, &r->gpu, md.verts, md.vcount, md.idx, md.icount);
        model::model_free(&md);

        if (!ok
            || !texture::texture_load(&m->tex, &r->gpu, tex_path)) {
            if (m->tex.tex) texture::texture_destroy(&m->tex, &r->gpu);
            gpu::gpu_shutdown(&r->gpu);
            return false;
        }

        r->Models.push_back(m);
        return true;
    }

    void renderer_add_renderable(Renderer* r, Renderable* renderable) {
        r->renderables.push_back(renderable);
    }

    void renderer_frame_tick(Renderer* r) {
        gpu::Frame f = gpu::frame_begin(&r->gpu);
        if (f.color && depth_ensure(&r->depth, &r->gpu, f.w, f.h)) {
            const float aspect = static_cast<float>(f.w) / static_cast<float>(f.h);

            const glm::vec3 eye(0, 2, 8);
            LightUBO light {
                glm::vec4(glm::normalize(glm::vec3(0.5f, 1.0f, 0.7f)), 0),
                  glm::vec4(1.0f, 0.95f, 0.9f, 0),
                  glm::vec4(0.08f, 0.08f, 0.1f, 0),
                  glm::vec4(eye, 1)
              };
            SDL_PushGPUFragmentUniformData(f.cmd, 0, &light, sizeof(light));

            const glm::mat4 view = glm::lookAt(eye, glm::vec3(0),
                                               glm::vec3(0, 1, 0));
            const glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
            const glm::mat4 vp   = proj * view;   // same for every object this frame

            SDL_GPURenderPass *pass =
                begin_main_pass(&f, &r->depth, SDL_FColor{0.1f, 0.1f, 0.15f, 1.0f});
            SDL_BindGPUGraphicsPipeline(pass, r->pipeline);

            for (Renderable* renderable : r->renderables) {
                RegisteredModel* rm = renderable->model;
                if (!rm) continue;

                texture::texture_bind(pass, &rm->tex, 0);

                const glm::mat4 model = glm::translate(glm::mat4(1.0f), renderable->position)
                                       * glm::mat4_cast(renderable->rotation);
                ObjectUBO obj;
                obj.model = model;
                obj.mvp = vp * model;
                obj.normal_mat = glm::mat4(glm::transpose(glm::inverse(glm::mat3(model))));
                SDL_PushGPUVertexUniformData(f.cmd, 0, &obj, sizeof(obj));
                mesh::mesh_draw(pass, &rm->mesh);
            }

            SDL_EndGPURenderPass(pass);
        }
        frame_end(&f);
    }
}