#include "renderer/renderer.hpp"

int main(int argc, char* argv[]) {
    tomato::renderer::Renderer r {};
    tomato::renderer::renderer_create(&r, "Engine");
    tomato::renderer::RegisteredModel cube {}, duck {};
    tomato::renderer::renderer_register_model(&r, &cube, "assets/BoxTextured.glb");
    tomato::renderer::renderer_register_model(&r, &duck, "assets/Duck.glb", "assets/XiJinping.png");

    tomato::renderer::Renderable box { &cube, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) };
    tomato::renderer::Renderable XiDuck {&duck, glm::vec3( 1, 0, 0), glm::quat(1.0f, 0.0f, 0.0f, 0.0f)};
    tomato::renderer::renderer_add_renderable(&r, &box);
    tomato::renderer::renderer_add_renderable(&r, &XiDuck);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.key) {
                    case SDLK_D: box.position.x += 0.1f; break;
                    case SDLK_A: box.position.x -= 0.1f; break;
                    case SDLK_W: box.position.y += 0.1f; break;
                    case SDLK_S: box.position.y -= 0.1f; break;
                    case SDLK_E: box.position.z += 0.1f; break;
                    case SDLK_Q: box.position.z -= 0.1f; break;
                }
            }
        }

        tomato::renderer::renderer_frame_tick(&r);
    }
}
