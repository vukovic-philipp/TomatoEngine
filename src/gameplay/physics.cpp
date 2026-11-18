#include "physics.hpp"
#include <SDL3/SDL.h>

#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
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

namespace tomato::physics {

    namespace { namespace L { constexpr JPH::ObjectLayer STATIC = 0, MOVING = 1; } }

    struct BPMap final : JPH::BroadPhaseLayerInterface {
        uint GetNumBroadPhaseLayers() const override { return 2;}
        JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer l) const override {
            return JPH::BroadPhaseLayer(1);
        }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
            switch ((JPH::BroadPhaseLayer::Type)layer) {
                case (JPH::BroadPhaseLayer::Type)L::STATIC: return "STATIC";
                case (JPH::BroadPhaseLayer::Type)L::MOVING: return "MOVING";
                default: return "INVALID";
            }
        }
#endif
    };

    struct ObjVsBP final : JPH::ObjectVsBroadPhaseLayerFilter {
        bool ShouldCollide(JPH::ObjectLayer o, JPH::BroadPhaseLayer bp) const override {
            return o == L::MOVING || bp == JPH::BroadPhaseLayer(L::MOVING);
        }
    };

    struct ObjPair final : JPH::ObjectLayerPairFilter {
        bool ShouldCollide(JPH::ObjectLayer a, JPH::ObjectLayer b) const override {
            return a == L::MOVING || b == L::MOVING;
        }   // static never hits static
    };


    struct Impl {
        JPH::TempAllocatorImpl temp{64 * 1024 * 1024};
        JPH::JobSystemThreadPool jobs{
            JPH::cMaxPhysicsJobs,
            JPH::cMaxPhysicsBarriers,
            static_cast<int>(std::thread::hardware_concurrency())};
        BPMap bp;
        ObjVsBP ovb;
        ObjPair opf;
        JPH::PhysicsSystem sys;
    };

    struct CharImpl {
        JPH::Ref<JPH::CharacterVirtual> ch;
    };

    bool world_create(World* w, const WorldConfig& conf) {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        w->impl = new Impl();
        w->impl->sys.Init(conf.max_bodies, 0, conf.max_bodies, conf.max_bodies, w->impl->bp,
            w->impl->ovb, w->impl->opf);
        w->impl->sys.SetGravity(conf.gravity);

        if (!JPH::Factory::sInstance) {
            SDL_Log("Jolt factory creation failed!");
            delete w->impl;
            JPH::UnregisterTypes();
            return false;
        }
        return true;
    }

    void world_step(World* w, float dt) {
        w->impl->sys.Update(dt, 1, &w->impl->temp, &w->impl->jobs);
    }

    void world_destroy(World* w) {
        delete w->impl;
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    uint32_t body_create_box(World* w, glm::vec3 pos, glm::vec3 half_extents, BodyType type) {
        JPH::BoxShapeSettings ShapeS(JPH::Vec3(half_extents.x, half_extents.y, half_extents.z));
        ShapeS.SetEmbedded();
        JPH::ShapeRefC shape = ShapeS.Create().Get();
        JPH::BodyCreationSettings bcs(shape, JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat::sIdentity(),
            type == BodyType::Static ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic,
            type == BodyType::Static ? L::STATIC : L::MOVING);
        JPH::BodyID id = w->impl->sys.GetBodyInterface().CreateAndAddBody(bcs, JPH::EActivation::Activate);
        return id.GetIndexAndSequenceNumber();
    }

    void body_get_transform(World* w, uint32_t handle, glm::vec3* pos, glm::quat* rot) {
        JPH::BodyID id(handle);
        auto& bi = w->impl->sys.GetBodyInterface();
        JPH::RVec3 p = bi.GetPosition(id);
        JPH::Quat  q = bi.GetRotation(id);
        *pos = { p.GetX(), p.GetY(), p.GetZ() };
        *rot = glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
    }

    void body_set_velocity(World* w, uint32_t handle, glm::vec3 v) {
        JPH::BodyID id(handle);
        w->impl->sys.GetBodyInterface().SetLinearVelocity(id, JPH::Vec3(v.x, v.y, v.z));
    }

    void character_create(World* w, Character* c, glm::vec3 pos, float radius, float height) {
        JPH::CharacterVirtualSettings s;
        s.mShape = JPH::RotatedTranslatedShapeSettings(
            JPH::Vec3(0, height * 0.05f, 0),
            JPH::Quat::sIdentity(),
            new JPH::CapsuleShape(height * 0.5f - radius, radius)).Create().Get();
        s.mUp = JPH::Vec3::sAxisY();
        s.mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
        s.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -radius);
        c->impl = new CharImpl();
        c->impl->ch = new JPH::CharacterVirtual(&s, JPH::RVec3(pos.x, pos.y, pos.z),
                                                JPH::Quat::sIdentity(), 0, &w->impl->sys);
    }

    void character_update(World* w, Character* c, glm::vec3 velocity, float dt) {
        c->impl->ch->SetLinearVelocity(JPH::Vec3(velocity.x, velocity.y, velocity.z));
        JPH::CharacterVirtual::ExtendedUpdateSettings eus;
        c->impl->ch->ExtendedUpdate(dt, w->impl->sys.GetGravity(), eus,
            w->impl->sys.GetDefaultBroadPhaseLayerFilter(L::MOVING),
            w->impl->sys.GetDefaultLayerFilter(L::MOVING),
            {}, {}, w->impl->temp);
    }

    glm::vec3 character_position(World* w, Character* c) {
        JPH::RVec3 p = c->impl->ch->GetPosition();
        return { p.GetX(), p.GetY(), p.GetZ() };
    }

    bool character_grounded(Character* c) {
        return c->impl->ch->GetGroundState() == JPH::CharacterVirtual::EGroundState::OnGround;
    }

}
