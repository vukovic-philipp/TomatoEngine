#pragma once

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Jolt/Jolt.h>

#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/PhysicsSettings.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"


JPH_SUPPRESS_WARNINGS

namespace tomato::physics {

    struct Impl;
    struct World {
        Impl* impl;
    };
    struct WorldConfig {
        JPH::Vec3 gravity{0, -9.81f, 0};
        uint32_t max_bodies = 65536;
    };

    enum BodyType : uint8_t { Static, Dynamic, Kinematic };

    bool world_create(World* w, const WorldConfig& conf);
    void world_destroy(World* w);
    void world_step(World* w, float dt);
    uint32_t body_create_box(World* w, glm::vec3 pos, glm::vec3 half_extents, BodyType type);
    void body_get_transform(World* w, uint32_t handle, glm::vec3* pos, glm::quat* rot);
    void body_set_velocity(World* w, uint32_t, glm::vec3 v);
}