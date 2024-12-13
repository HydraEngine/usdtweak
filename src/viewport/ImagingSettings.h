#pragma once
#include "runtime/engine.h"
#include "runtime/renderParams.h"
#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/usd/usdGeom/camera.h>

PXR_NAMESPACE_USING_DIRECTIVE

struct ImagingSettings : runtime::UsdImagingGLRenderParams {
    ImagingSettings();

    void SetLightPositionFromCamera(const GfCamera &);

    // Defaults GL lights and materials
    bool enableCameraLight;
    const GlfSimpleLightVector &GetLights();

    GlfSimpleMaterial _material;
    GfVec4f _ambient;

    // Viewport
    bool showGrid;
    bool showGizmos;
    bool showUI;
    bool showViewportMenu;

private:
    GlfSimpleLightVector _lights;
};

/// We keep track of the selected AOV in the UI, unfortunately the selected AOV is not awvailable in
/// RuntimeEngine, so we need the initialize the UI data with this function
void InitializeRendererAov(runtime::RuntimeEngine &);

///
void DrawRendererSelectionCombo(runtime::RuntimeEngine &);
void DrawRendererSelectionList(runtime::RuntimeEngine &);
void DrawRendererControls(runtime::RuntimeEngine &);
void DrawRendererCommands(runtime::RuntimeEngine &);
void DrawRendererSettings(runtime::RuntimeEngine &, ImagingSettings &);
void DrawImagingSettings(runtime::RuntimeEngine &, ImagingSettings &);
void DrawAovSettings(runtime::RuntimeEngine &);
void DrawColorCorrection(runtime::RuntimeEngine &, ImagingSettings &);
