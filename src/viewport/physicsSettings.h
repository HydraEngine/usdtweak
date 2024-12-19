//  Copyright (c) 2024 Feng Yang
//
//  I am making my contributions/submissions to this project solely in my
//  personal capacity and am not conveying any rights to any intellectual
//  property of any third parties.

#pragma once

#include "fabric_sim/physxEngine.h"

struct PhysicsSettings {
    bool update{true};

    float scale = 0;
    bool world_axes{false};
    bool body_axes{false};
    bool body_mass_axes{false};
    bool body_lin_velocity{false};
    bool body_ang_velocity{false};
    bool contact_point{false};
    bool contact_normal{false};
    bool contact_error{false};
    bool contact_impulse{false};
    bool friction_point{false};
    bool friction_normal{false};
    bool friction_impulse{false};
    bool actor_axes{false};
    bool collision_aabbs{false};
    bool collision_shapes{true};
    bool collision_axes{false};
    bool collision_compounds{false};
    bool collision_face_normals{false};
    bool collision_edges{false};
    bool collision_static{false};
    bool collision_dynamic{false};
    bool joint_local_frames{true};
    bool joint_limits{false};
    bool cull_box{false};
    bool mbp_regions{false};
    bool simulation_mesh{false};
    bool sdf{false};

    void DrawSettings();

    void Sync(sim::PhysxEngine& engine);
};