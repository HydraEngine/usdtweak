//  Copyright (c) 2024 Feng Yang
//
//  I am making my contributions/submissions to this project solely in my
//  personal capacity and am not conveying any rights to any intellectual
//  property of any third parties.

#include "physicsSettings.h"
#include "fabric_sim/tokens.h"
#include "Gui.h"

void PhysicsSettings::DrawSettings() {
    ImGui::Checkbox("Enable Update", &update);

    ImGui::Separator();

    ImGui::SliderFloat("Debug Scale", &scale, 0.0f, 10.0f, "ratio = %.3f");
    ImGui::Checkbox("Show World Axes", &world_axes);
    ImGui::Checkbox("Show Body Axes", &body_axes);
    ImGui::Checkbox("Show Mass Axes", &body_mass_axes);
    ImGui::Checkbox("Show Lin Velocity", &body_lin_velocity);
    ImGui::Checkbox("Show Angular Velocity", &body_ang_velocity);
    ImGui::Checkbox("Show Contact Point", &contact_point);
    ImGui::Checkbox("Show Contact Normal", &contact_normal);
    ImGui::Checkbox("Show Contact Error", &contact_error);
    ImGui::Checkbox("Show Contact Impulse", &contact_impulse);
    ImGui::Checkbox("Show Friction Point", &friction_point);
    ImGui::Checkbox("Show Friction Normal", &friction_normal);
    ImGui::Checkbox("Show Friction Impulse", &friction_impulse);
    ImGui::Checkbox("Show Actor Axes", &actor_axes);
    ImGui::Checkbox("Show Collision AABBS", &collision_aabbs);
    ImGui::Checkbox("Show Collision Shapes", &collision_shapes);
    ImGui::Checkbox("Show Collision Axes", &collision_axes);
    ImGui::Checkbox("Show Collision Compounds", &collision_compounds);
    ImGui::Checkbox("Show Collision Face Normals", &collision_face_normals);
    ImGui::Checkbox("Show Collision Edges", &collision_edges);
    ImGui::Checkbox("Show Collision Static", &collision_static);
    ImGui::Checkbox("Show Collision Dynamic", &collision_dynamic);
    ImGui::Checkbox("Show Joint Local Frames", &joint_local_frames);
    ImGui::Checkbox("Show Joint Limits", &joint_limits);
    ImGui::Checkbox("Show Cull Box", &cull_box);
    ImGui::Checkbox("Show MBP Regions", &mbp_regions);
    ImGui::Checkbox("Show Simulation Mesh", &simulation_mesh);
    ImGui::Checkbox("Show SDF", &sdf);
}

void PhysicsSettings::Sync(sim::PhysxEngine& engine) {
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eSCALE, scale);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eWORLD_AXES, world_axes);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eBODY_AXES, body_axes);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eBODY_MASS_AXES, body_mass_axes);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eBODY_LIN_VELOCITY, body_lin_velocity);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eBODY_ANG_VELOCITY, body_ang_velocity);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCONTACT_POINT, contact_point);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCONTACT_NORMAL, contact_normal);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCONTACT_ERROR, contact_error);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCONTACT_IMPULSE, contact_impulse);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eFRICTION_POINT, friction_point);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eFRICTION_NORMAL, friction_normal);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eFRICTION_IMPULSE, friction_impulse);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eACTOR_AXES, actor_axes);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCOLLISION_AABBS, collision_aabbs);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCOLLISION_SHAPES, collision_shapes);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCOLLISION_AXES, collision_axes);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCOLLISION_COMPOUNDS, collision_compounds);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCOLLISION_FNORMALS, collision_face_normals);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCOLLISION_EDGES, collision_edges);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCOLLISION_STATIC, collision_static);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCOLLISION_DYNAMIC, collision_dynamic);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eJOINT_LOCAL_FRAMES, joint_local_frames);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eJOINT_LIMITS, joint_limits);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eCULL_BOX, cull_box);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eMBP_REGIONS, mbp_regions);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eSIMULATION_MESH, simulation_mesh);
    engine.SetVisualizationParameter(pxr::FabricSimTokens->eSDF, sdf);
}