#pragma once
#include <glm/vec2.hpp>

#include "physics.hpp"

namespace tomato::player {
    struct PlayerCmd {
        glm::vec2 move;
        bool jump;
    };

    struct Player {
        physics::Character body;
        glm::vec3 pos, prev_pos;
        glm::vec3 vel;
        float yaw, pitch;
        bool grounded;
    };

    void player_create(Player* p, physics::World* w, glm::vec3 pos);
    void player_tick(Player* p, physics::World* w, const PlayerCmd& c, float dt);
    glm::vec3 player_eye(const Player* p, float alpha);
}
