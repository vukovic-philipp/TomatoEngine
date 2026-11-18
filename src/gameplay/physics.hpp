#pragma once

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Jolt/Jolt.h>

#include "Jolt/Physics/Collision/Shape/MeshShape.h"


JPH_SUPPRESS_WARNINGS

namespace tomato::physics {

    // DEFAULT_PHYSICS_TICK_SPEED (60 TicksPerSecond)
    constexpr float DPTS = 1.0f / 60.0f;

    struct Impl;
    struct World {
        Impl* impl;
    };
    struct WorldConfig {
        JPH::Vec3 gravity{0, -9.81f, 0};
        uint32_t max_bodies = 65536;
    };

    struct CharImpl;
    struct Character {
        CharImpl* impl;
    };

    enum BodyType : uint8_t { Static, Dynamic, Kinematic };

    bool world_create(World* w, const WorldConfig& conf);
    void world_destroy(World* w);
    void world_step(World* w, float dt);

    uint32_t body_create_box(World* w, glm::vec3 pos, glm::vec3 half_extents, BodyType type);

    void body_get_transform(World* w, uint32_t handle, glm::vec3* pos, glm::quat* rot);
    void body_set_velocity(World* w, uint32_t, glm::vec3 v);

    void character_create(World* w, Character* c, glm::vec3 pos, float radius, float height);
    void character_update(World* w, Character* c, glm::vec3 velocity, float dt);
    glm::vec3 character_position(World* w, Character* c);
    bool character_grounded(Character*);
}