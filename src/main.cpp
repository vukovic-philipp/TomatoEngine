#include "gameplay/physics.hpp"
#include "renderer/renderer.hpp"

int main(int argc, char* argv[]) {
    tomato::renderer::Renderer r {};
    tomato::renderer::renderer_create(&r, "Engine");
    tomato::renderer::RegisteredModel cube {}, duck {};
    tomato::renderer::renderer_register_model(&r, &cube, "assets/BoxTextured.glb");
    tomato::renderer::renderer_register_model(&r, &duck, "assets/Duck.glb", "assets/XiJinping.png");

    tomato::renderer::Renderable box { &cube, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 0.0f) };
    tomato::renderer::Renderable fallingBox { &cube, glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) };
    tomato::renderer::Renderable XiDuck {&duck, glm::vec3( -2, 0, 4), glm::vec3(0.0f, 0.0f, 0.0f)};
    tomato::renderer::renderer_add_renderable(&r, &box);
    tomato::renderer::renderer_add_renderable(&r, &fallingBox);
    tomato::renderer::renderer_add_renderable(&r, &XiDuck);

    tomato::physics::World pw {};
    tomato::physics::WorldConfig wc {};
    tomato::physics::world_create(&pw, wc);

    const glm::vec3 half_extents(0.5f);

    uint32_t box_body = tomato::physics::body_create_box(&pw, box.position, half_extents, tomato::physics::BodyType::Dynamic);
    uint32_t falling_body = tomato::physics::body_create_box(&pw, fallingBox.position, half_extents, tomato::physics::BodyType::Dynamic);
    uint32_t static_body = tomato::physics::body_create_box(&pw, glm::vec3(0.0f, -15.0f, 0.0f), glm::vec3(12.5f), tomato::physics::Static);

    const float dt = 1.0f / 50.0f;
    const float move_speed = 3.0f;
    float acc = 0; uint64_t last = SDL_GetTicksNS();

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.key) {
                    case SDLK_O: XiDuck.rotation.y += 0.1f; break;
                    case SDLK_P: XiDuck.rotation.y -= 0.1f; break;
                }
            }
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);
        glm::vec3 vel(0.0f);
        bool moving = false;
        if (keys[SDL_SCANCODE_D]) { vel.x += move_speed; moving = true; }
        if (keys[SDL_SCANCODE_A]) { vel.x -= move_speed; moving = true; }
        if (keys[SDL_SCANCODE_W]) { vel.y += move_speed; moving = true; }
        if (keys[SDL_SCANCODE_S]) { vel.y -= move_speed; moving = true; }
        if (keys[SDL_SCANCODE_E]) { vel.z += move_speed; moving = true; }
        if (keys[SDL_SCANCODE_Q]) { vel.z -= move_speed; moving = true; }
        if (moving) tomato::physics::body_set_velocity(&pw, box_body, vel);

        uint64_t now = SDL_GetTicksNS();
        acc += (now - last) * 1e-9f; last = now;
        if (acc > 0.25f) acc = 0.25f;                 // avoids the spiral of death after a stall
        while (acc >= dt) { tomato::physics::world_step(&pw, dt); acc -= dt; }

        glm::quat box_rot, falling_rot;
        tomato::physics::body_get_transform(&pw, box_body, &box.position, &box_rot);
        box.rotation = glm::eulerAngles(box_rot);
        tomato::physics::body_get_transform(&pw, falling_body, &fallingBox.position, &falling_rot);
        fallingBox.rotation = glm::eulerAngles(falling_rot);

        tomato::renderer::renderer_frame_tick(&r);
    }

    tomato::physics::world_destroy(&pw);
}
