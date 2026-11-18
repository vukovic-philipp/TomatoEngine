#include "player.hpp"
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace tomato::player {
    namespace {
        constexpr float MAX_SPEED = 5.0f;
        constexpr float GROUND_ACCEL = 10.0f;
        constexpr float AIR_ACCEL = 2.0f;
        constexpr float JUMP_SPEED = 5.0f;
        constexpr float GRAVITY = -9.81f;
        constexpr float PLAYER_RADIUS = 0.3f;
        constexpr float PLAYER_HEIGHT = 1.8f;
        constexpr float EYE_HEIGHT = 1.6f;
    }

    void player_create(Player* p, physics::World* w, glm::vec3 pos) {
        p->pos = pos;
        p->prev_pos = pos;
        p->vel = glm::vec3(0.0f);
        p->yaw = 0.0f;
        p->pitch = 0.0f;
        p->grounded = false;
        physics::character_create(w, &p->body, pos, PLAYER_RADIUS, PLAYER_HEIGHT);
    }

    void player_tick(Player* p, physics::World* w, const PlayerCmd& c, float dt) {
        p->prev_pos = p->pos;
        p->grounded = physics::character_grounded(&p->body);
        glm::vec3 fwd(sinf(p->yaw), 0, -cosf(p->yaw));
        glm::vec3 right(-fwd.z, 0, fwd.x);
        glm::vec3 wish = fwd * c.move.y + right * c.move.x;
        if (glm::length(wish) > 1.0f) wish = glm::normalize(wish);

        // horizontal: accelerate toward target (ground = snappy, air = weak)
        glm::vec3 target = wish * MAX_SPEED;
        float accel = p->grounded ? GROUND_ACCEL : AIR_ACCEL;
        glm::vec3 h(p->vel.x, 0, p->vel.z);
        h += (target - h) * glm::min(accel * dt, 1.0f);   // simple version; Quake-style later
        p->vel.x = h.x; p->vel.z = h.z;

        // vertical
        if (p->grounded) { p->vel.y = c.jump ? JUMP_SPEED : 0.0f; }
        else             { p->vel.y += GRAVITY * dt; }

        physics::character_update(w, &p->body, p->vel, dt);
        p->pos = physics::character_position(w, &p->body);
    }

    glm::vec3 player_eye(const Player* p, float alpha) {
        glm::vec3 interp = glm::mix(p->prev_pos, p->pos, alpha);
        return interp + glm::vec3(0.0f, EYE_HEIGHT, 0.0f);
    }
}