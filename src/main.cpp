#include "gameplay/physics.hpp"
#include "gameplay/player.hpp"
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

    SDL_SetWindowRelativeMouseMode(r.gpu.window, true);

    tomato::physics::World pw {};
    tomato::physics::WorldConfig wc {};
    tomato::physics::world_create(&pw, wc);

    const glm::vec3 half_extents(0.5f);

    uint32_t box_body = tomato::physics::body_create_box(&pw, box.position, half_extents, tomato::physics::BodyType::Dynamic);
    uint32_t falling_body = tomato::physics::body_create_box(&pw, fallingBox.position, half_extents, tomato::physics::BodyType::Dynamic);
    uint32_t static_body = tomato::physics::body_create_box(&pw, glm::vec3(0.0f, -15.0f, 0.0f), glm::vec3(12.5f), tomato::physics::Static);

    tomato::player::Player player {};
    tomato::player::player_create(&player, &pw, glm::vec3(0.0f, 1.0f, -5.0f));

    constexpr float MOUSE_SENSITIVITY = 0.0025f;
    constexpr float PITCH_LIMIT = glm::radians(89.0f);
    float acc = 0; uint64_t last = SDL_GetTicksNS();

    int fps_frames = 0; float fps_timer = 0.0f;

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
            if (e.type == SDL_EVENT_MOUSE_MOTION) {
                player.yaw += e.motion.xrel * MOUSE_SENSITIVITY;
                player.pitch = glm::clamp(player.pitch - e.motion.yrel * MOUSE_SENSITIVITY,
                                          -PITCH_LIMIT, PITCH_LIMIT);
            }
        }

        const bool* keys = SDL_GetKeyboardState(nullptr);

        tomato::player::PlayerCmd cmd {};
        if (keys[SDL_SCANCODE_D]) cmd.move.x += 1.0f;
        if (keys[SDL_SCANCODE_A]) cmd.move.x -= 1.0f;
        if (keys[SDL_SCANCODE_W]) cmd.move.y += 1.0f;
        if (keys[SDL_SCANCODE_S]) cmd.move.y -= 1.0f;
        cmd.jump = keys[SDL_SCANCODE_SPACE];

        uint64_t now = SDL_GetTicksNS();
        float frame_dt = (now - last) * 1e-9f; last = now;

        acc += frame_dt;
        if (acc > 0.25f) acc = 0.25f;                 // avoids the spiral of death after a stall
        while (acc >= tomato::physics::DPTS) {
            tomato::player::player_tick(&player, &pw, cmd, tomato::physics::DPTS);
            tomato::physics::world_step(&pw, tomato::physics::DPTS);
            acc -= tomato::physics::DPTS;
        }

        tomato::renderer::renderer_set_camera(&r, tomato::player::player_eye(&player, 1.0f),
                                              player.yaw, player.pitch);

        glm::quat box_rot, falling_rot;
        tomato::physics::body_get_transform(&pw, box_body, &box.position, &box_rot);
        box.rotation = glm::eulerAngles(box_rot);

        tomato::physics::body_get_transform(&pw, falling_body, &fallingBox.position, &falling_rot);
        fallingBox.rotation = glm::eulerAngles(falling_rot);

        tomato::renderer::renderer_frame_tick(&r);

        fps_frames++;
        fps_timer += frame_dt;
        if (fps_timer >= 1.0f) {
            SDL_Log("FPS: %d", fps_frames);
            fps_frames = 0;
            fps_timer = 0.0f;
        }
    }

    tomato::physics::world_destroy(&pw);
}
