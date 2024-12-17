#include <iostream>

#include <pxr/imaging/garch/glApi.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usdGeom/boundable.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include "Commands.h"
#include "Constants.h"
#include "Gui.h"
#include "ImGuiHelpers.h"
#include "Shortcuts.h"
#include "UsdPrimEditor.h" // DrawUsdPrimEditTarget
#include "Viewport.h"

namespace clk = std::chrono;

// TODO: picking meshes: https://groups.google.com/g/usd-interest/c/P2CynIu7MYY/m/UNPIKzmMBwAJ

void Viewport::DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Renderer")) {
            if (_renderer) {
                DrawRendererControls(*_renderer);
                DrawRendererSelectionCombo(*_renderer);
                DrawColorCorrection(*_renderer, _imagingSettings);
                DrawAovSettings(*_renderer);
                DrawRendererCommands(*_renderer);
                if (ImGui::BeginMenu("Renderer Settings")) {
                    DrawRendererSettings(*_renderer, _imagingSettings);
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Viewport")) {
            if (_renderer) {
                DrawImagingSettings(*_renderer, _imagingSettings);
                ImGui::Checkbox("Show UI", &_imagingSettings.showUI);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Cameras")) {
            if (_renderer) {
                _cameras.DrawCameraList(GetCurrentStage());
                _cameras.DrawCameraEditor(GetCurrentStage(), GetCurrentTimeCode());
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

Viewport::Viewport(UsdStageRefPtr stage, Selection &selection)
    : _stage(stage), _cameraManipulator({InitialWindowWidth, InitialWindowHeight}),
      _currentEditingState(new MouseHoverManipulator()), _activeManipulator(&_positionManipulator), _selection(selection),
      _textureSize(1, 1), _viewportName("Viewport 1") {

    // Viewport draw target
    _cameraManipulator.ResetPosition(GetEditableCamera());

    _drawTarget = GlfDrawTarget::New(_textureSize, false);
    _drawTarget->Bind();
    _drawTarget->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);
    _drawTarget->AddAttachment("depth", GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_COMPONENT32F);
    auto color = _drawTarget->GetAttachment("color");
    _textureId = color->GetGlTextureName();
    _drawTarget->Unbind();
}

Viewport::~Viewport() {
    if (_renderer) {
        _renderer = nullptr; // will be deleted in the map
    }
    // Delete renderers
    _drawTarget->Bind();
    _renderer = nullptr;
    _renderStage = nullptr;
    _drawTarget->Unbind();
}

static void DrawOpenedStages() {
    ScopedStyleColor defaultStyle(DefaultColorStyle);
    const UsdStageCache &stageCache = UsdUtilsStageCache::Get();
    const auto allStages = stageCache.GetAllStages();
    for (const auto &stagePtr : allStages) {
        if (ImGui::MenuItem(stagePtr->GetRootLayer()->GetIdentifier().c_str())) {
            ExecuteAfterDraw<EditorSetCurrentStage>(stagePtr->GetRootLayer());
        }
    }
}

/// Draw the viewport widget
void Viewport::Draw() {
    if (_imagingSettings.showViewportMenu) {
        DrawMenuBar();
    }
    const ImVec2 wsize = ImGui::GetWindowSize();
    // Set the size of the texture here as we need the current window size
    const auto cursorPos = ImGui::GetCursorPos();
    _textureSize = GfVec2i(std::max(1.f, wsize[0]), std::max(1.f, wsize[1] - cursorPos.y));

    if (_textureId) {
        // Get the size of the child (i.e. the whole draw size of the windows).
        ImGui::Image((ImTextureID)((uintptr_t)_textureId), ImVec2(_textureSize[0], _textureSize[1]), ImVec2(0, 1), ImVec2(1, 0));
        // TODO: it is possible to have a popup menu on top of the viewport.
        // It should be created depending on the manipulator/editor state
        // if (ImGui::BeginPopupContextItem()) {
        //    ImGui::Button("ColorCorrection");
        //    ImGui::Button("Deactivate");
        //    ImGui::EndPopup();
        //}
        HandleManipulationEvents();
        HandleKeyboardShortcut();
        if (_imagingSettings.showUI) {
            ImGui::BeginDisabled(!bool(GetCurrentStage()));
            DrawToolBar(cursorPos + ImVec2(15, 15));
            ImGui::EndDisabled();
        }
    }
}

void Viewport::DrawToolBar(const ImVec2 widgetPosition) {
    const ImVec2 buttonSize(25, 25); // Button size
    const ImVec4 defaultColor(0.1, 0.1, 0.1, 0.7);
    const ImVec4 selectedColor(ColorButtonHighlight);

    ImGui::SetCursorPos(widgetPosition);
    ImGui::PushStyleColor(ImGuiCol_Button, defaultColor);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, defaultColor);
    ImGuiPopupFlags flags = ImGuiPopupFlags_MouseButtonLeft;
    DrawPickMode(_selectionManipulator);
    ImGui::SameLine();
    ImGui::Button(ICON_FA_USER_COG);
    if (_renderer && ImGui::BeginPopupContextItem(nullptr, flags)) {
        DrawRendererControls(*_renderer);
        DrawRendererSelectionCombo(*_renderer);
        DrawColorCorrection(*_renderer, _imagingSettings);
        DrawAovSettings(*_renderer);
        DrawRendererCommands(*_renderer);
        if (ImGui::BeginMenu("Renderer Settings")) {
            DrawRendererSettings(*_renderer, _imagingSettings);
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
    if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1) {
        ImGui::SetTooltip("Renderer settings");
    }
    ImGui::SameLine();
    ImGui::Button(ICON_FA_TV);
    if (_renderer && ImGui::BeginPopupContextItem(nullptr, flags)) {
        DrawImagingSettings(*_renderer, _imagingSettings);
        ImGui::Checkbox("Show menu bar", &_imagingSettings.showViewportMenu);
        ImGui::EndPopup();
    }
    if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1) {
        ImGui::SetTooltip("Viewport settings");
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, _imagingSettings.enableCameraLight ? selectedColor : defaultColor);
    if (ImGui::Button(ICON_FA_FIRE)) {
        _imagingSettings.enableCameraLight = !_imagingSettings.enableCameraLight;
    }
    if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1) {
        ImGui::SetTooltip("Camera light on/off");
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, _imagingSettings.enableSceneMaterials ? selectedColor : defaultColor);
    if (ImGui::Button(ICON_FA_HAND_SPARKLES)) {
        _imagingSettings.enableSceneMaterials = !_imagingSettings.enableSceneMaterials;
    }
    if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1) {
        ImGui::SetTooltip("Scene materials on/off");
    }
    ImGui::PopStyleColor();
    if (_renderer && _renderer->GetRendererPlugins().size() >= 2) {
        ImGui::SameLine();
        ImGui::Button(_renderer->GetRendererDisplayName(_renderer->GetCurrentRendererId()).c_str());
        if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1) {
            ImGui::SetTooltip("Render delegate");
        }
        if (ImGui::BeginPopupContextItem(nullptr, flags)) {
            DrawRendererSelectionList(*_renderer);
            ImGui::EndPopup();
        }
    }
    ImGui::SameLine();
    std::string cameraName(ICON_FA_CAMERA);
    cameraName += "  " + _cameras.GetCurrentCameraName();
    ImGui::Button(cameraName.c_str());
    if (_renderer &&
        ImGui::BeginPopupContextItem(_viewportName.c_str(), flags)) { // should be name with the viewport name instead
        _cameras.DrawCameraList(GetCurrentStage());
        _cameras.DrawCameraEditor(GetCurrentStage(), GetCurrentTimeCode());
        ImGui::EndPopup();
    }
    if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 1) {
        ImGui::SetTooltip("Cameras");
    }
    ImGui::SameLine();
    ImGui::Checkbox(ICON_FA_ROBOT, &_imagingSettings.play);

    ImGui::PopStyleColor(2);
}

// Poor man manipulator toolbox
void Viewport::DrawManipulatorToolbox(const ImVec2 widgetPosition) {
    const ImVec2 buttonSize(25, 25); // Button size
    const ImVec4 defaultColor(0.1, 0.1, 0.1, 0.9);
    const ImVec4 selectedColor(ColorButtonHighlight);
    ImGui::SetCursorPos(widgetPosition);
    ImGui::SetNextItemWidth(80);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, defaultColor);
    DrawPickMode(_selectionManipulator);
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, IsChosenManipulator<MouseHoverManipulator>() ? selectedColor : defaultColor);
    ImGui::SetCursorPosX(widgetPosition.x);

    if (ImGui::Button(ICON_FA_LOCATION_ARROW, buttonSize)) {
        ExecuteAfterDraw<ViewportsSelectMouseHoverManipulator>();
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, IsChosenManipulator<PositionManipulator>() ? selectedColor : defaultColor);
    ImGui::SetCursorPosX(widgetPosition.x);
    if (ImGui::Button(ICON_FA_ARROWS_ALT, buttonSize)) {
        ExecuteAfterDraw<ViewportsSelectPositionManipulator>();
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, IsChosenManipulator<RotationManipulator>() ? selectedColor : defaultColor);
    ImGui::SetCursorPosX(widgetPosition.x);
    if (ImGui::Button(ICON_FA_SYNC_ALT, buttonSize)) {
        ExecuteAfterDraw<ViewportsSelectRotationManipulator>();
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, IsChosenManipulator<ScaleManipulator>() ? selectedColor : defaultColor);
    ImGui::SetCursorPosX(widgetPosition.x);
    if (ImGui::Button(ICON_FA_COMPRESS, buttonSize)) {
        ExecuteAfterDraw<ViewportsSelectScaleManipulator>();
    }
    ImGui::PopStyleColor();
}

/// Frame the viewport using the bounding box of the selection
void Viewport::FrameCameraOnSelection(const Selection &selection) { // Camera manipulator ???
    if (GetCurrentStage() && !selection.IsSelectionEmpty(GetCurrentStage())) {
        UsdGeomBBoxCache bboxcache(_imagingSettings.frame, UsdGeomImageable::GetOrderedPurposeTokens());
        GfBBox3d bbox;
        for (const auto &primPath : selection.GetSelectedPaths(GetCurrentStage())) {
            bbox = GfBBox3d::Combine(bboxcache.ComputeWorldBound(GetCurrentStage()->GetPrimAtPath(primPath)), bbox);
        }
        auto defaultPrim = GetCurrentStage()->GetDefaultPrim();
        _cameraManipulator.FrameBoundingBox(GetEditableCamera(), bbox);
    }
}

/// Frame the viewport using the bounding box of the root prim
void Viewport::FrameCameraOnRootPrim() {
    if (GetCurrentStage()) {
        UsdGeomBBoxCache bboxcache(_imagingSettings.frame, UsdGeomImageable::GetOrderedPurposeTokens());
        auto defaultPrim = GetCurrentStage()->GetDefaultPrim();
        if (defaultPrim) {
            _cameraManipulator.FrameBoundingBox(GetEditableCamera(), bboxcache.ComputeWorldBound(defaultPrim));
        } else {
            auto rootPrim = GetCurrentStage()->GetPrimAtPath(SdfPath("/"));
            _cameraManipulator.FrameBoundingBox(GetEditableCamera(), bboxcache.ComputeWorldBound(rootPrim));
        }
    }
}

void Viewport::FrameAllCameras() {
    if (GetCurrentStage()) {
        UsdGeomBBoxCache bboxcache(_imagingSettings.frame, UsdGeomImageable::GetOrderedPurposeTokens());
        auto defaultPrim = GetCurrentStage()->GetDefaultPrim();
        if (defaultPrim) {
            for (GfCamera *camera : _cameras.GetEditableCameras(GetCurrentStage())) {
                _cameraManipulator.FrameBoundingBox(*camera, bboxcache.ComputeWorldBound(defaultPrim));
            }
        } else {
            for (GfCamera *camera : _cameras.GetEditableCameras(GetCurrentStage())) {
                auto rootPrim = GetCurrentStage()->GetPrimAtPath(SdfPath("/"));
                _cameraManipulator.FrameBoundingBox(*camera, bboxcache.ComputeWorldBound(rootPrim));
            }
        }
    }
}

GfVec2d Viewport::GetPickingBoundarySize() const {
    const GfVec2i renderSize = _drawTarget->GetSize();
    const double width = static_cast<double>(renderSize[0]);
    const double height = static_cast<double>(renderSize[1]);
    return GfVec2d(20.0 / width, 20.0 / height);
}

//
double Viewport::ComputeScaleFactor(const GfVec3d &objectPos, const double multiplier) const {
    double scale = 1.0;
    const auto &frustum = GetCurrentCamera().GetFrustum();
    auto ray = frustum.ComputeRay(GfVec2d(0, 0)); // camera axis
    ray.FindClosestPoint(objectPos, &scale);
    // TODO Ortho case: should the scale be based on the larger/smaller side ?
    if (GetCurrentCamera().GetProjection() == GfCamera::Orthographic) {
        const float verticalAperture = GetCurrentCamera().GetVerticalAperture();
        scale = 0.01 * verticalAperture;

    } else {
        const float focalLength = GetCurrentCamera().GetFocalLength();
        scale /= focalLength == 0 ? 1.f : focalLength;
    }
    scale /= multiplier;
    scale *= 2;
    return scale;
}

inline bool IsModifierDown() { return ImGui::GetIO().KeyMods != 0; }

void Viewport::HandleKeyboardShortcut() {
    if (ImGui::IsItemHovered()) {
        ImGuiIO &io = ImGui::GetIO();
        static bool SelectionManipulatorPressedOnce = true;
        if (ImGui::IsKeyDown(ImGuiKey_Q) && !IsModifierDown()) {
            if (SelectionManipulatorPressedOnce) {
                ExecuteAfterDraw<ViewportsSelectMouseHoverManipulator>();
                SelectionManipulatorPressedOnce = false;
            }
        } else {
            SelectionManipulatorPressedOnce = true;
        }

        static bool PositionManipulatorPressedOnce = true;
        if (ImGui::IsKeyDown(ImGuiKey_W) && !IsModifierDown()) {
            if (PositionManipulatorPressedOnce) {
                ExecuteAfterDraw<ViewportsSelectPositionManipulator>();
                PositionManipulatorPressedOnce = false;
            }
        } else {
            PositionManipulatorPressedOnce = true;
        }

        static bool RotationManipulatorPressedOnce = true;
        if (ImGui::IsKeyDown(ImGuiKey_E) && !IsModifierDown()) {
            if (RotationManipulatorPressedOnce) {
                ExecuteAfterDraw<ViewportsSelectRotationManipulator>();
                RotationManipulatorPressedOnce = false;
            }
        } else {
            RotationManipulatorPressedOnce = true;
        }

        static bool ScaleManipulatorPressedOnce = true;
        if (ImGui::IsKeyDown(ImGuiKey_R) && !IsModifierDown()) {
            if (ScaleManipulatorPressedOnce) {
                ExecuteAfterDraw<ViewportsSelectScaleManipulator>();
                ScaleManipulatorPressedOnce = false;
            }
        } else {
            ScaleManipulatorPressedOnce = true;
        }

        // Playback
        AddShortcut<EditorTogglePlayback, ImGuiKey_Space>();
    }
}

void Viewport::HandleManipulationEvents() {

    ImGuiContext *g = ImGui::GetCurrentContext();
    ImGuiIO &io = ImGui::GetIO();

    // Check the mouse is over this widget
    if (ImGui::IsItemHovered()) {
        const GfVec2i drawTargetSize = _drawTarget->GetSize();
        if (drawTargetSize[0] == 0 || drawTargetSize[1] == 0)
            return;
        _mousePosition[0] =
            2.0 * (static_cast<double>(io.MousePos.x - (g->LastItemData.Rect.Min.x)) / static_cast<double>(drawTargetSize[0])) -
            1.0;
        _mousePosition[1] =
            -2.0 * (static_cast<double>(io.MousePos.y - (g->LastItemData.Rect.Min.y)) / static_cast<double>(drawTargetSize[1])) +
            1.0;

        /// This works like a Finite state machine
        /// where every manipulator/editor is a state
        if (!_currentEditingState) {
            _currentEditingState = GetManipulator<MouseHoverManipulator>();
            _currentEditingState->OnBeginEdition(*this);
        }

        auto newState = _currentEditingState->OnUpdate(*this);
        if (newState != _currentEditingState) {
            _currentEditingState->OnEndEdition(*this);
            _currentEditingState = newState;
            _currentEditingState->OnBeginEdition(*this);
        }
    } else { // Mouse is outside of the viewport, reset the state
        if (_currentEditingState) {
            _currentEditingState->OnEndEdition(*this);
            _currentEditingState = nullptr;
        }
    }
}

GfVec2i Viewport::GetViewportSize() const { return _drawTarget->GetSize(); }

GfCamera &Viewport::GetEditableCamera() { return _cameras.GetEditableCamera(); }
const GfCamera &Viewport::GetCurrentCamera() const { return _cameras.GetCurrentCamera(); }

// TODO: keep the viewport camera in a variable and avoid recomputing it when not necessary
GfCamera Viewport::GetViewportCamera(double width, double height) const {
    GfCamera viewportCamera = GetCurrentCamera();

    if (viewportCamera.GetProjection() == GfCamera::Perspective) {
        viewportCamera.SetPerspectiveFromAspectRatioAndFieldOfView(
            width / height, viewportCamera.GetFieldOfView(GfCamera::FOVHorizontal), GfCamera::FOVHorizontal);
    } else { // assuming ortho
        viewportCamera.SetOrthographicFromAspectRatioAndSize(
            width / height, viewportCamera.GetHorizontalAperture() * GfCamera::APERTURE_UNIT, GfCamera::FOVHorizontal);
    }

    return viewportCamera;
}

// TODO: keep the viewport camera in the structure and return a const ref
GfCamera Viewport::GetViewportCamera() const {
    GfVec2i renderSize = _drawTarget->GetSize();
    const int width = renderSize[0];
    const int height = renderSize[1];
    return GetViewportCamera(width, height);
}

void Viewport::BeginHydraUI(int width, int height) {
    // Create a ImGui windows to render the gizmos in
    ImGui_ImplOpenGL3_NewFrame();
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    ImGui::NewFrame();
    static bool alwaysOpened = true;
    constexpr ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_None;
    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    // Full screen invisible window
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowBgAlpha(0.0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("HydraHUD", &alwaysOpened, windowFlags);
    ImGui::PopStyleVar(3);
}

void Viewport ::EndHydraUI() { ImGui::End(); }

void Viewport::Render() {
    GfVec2i renderSize = _drawTarget->GetSize();
    int width = renderSize[0];
    int height = renderSize[1];

    if (width == 0 || height == 0)
        return;

    // Draw active manipulator and HUD
    if (_imagingSettings.showGizmos) {
        BeginHydraUI(width, height);
        GetActiveManipulator().OnDrawFrame(*this);
        // DrawHUD(this);
        EndHydraUI();
    }

    _drawTarget->Bind();
    glEnable(GL_DEPTH_TEST);
    glClearColor(_imagingSettings.clearColor[0], _imagingSettings.clearColor[1], _imagingSettings.clearColor[2],
                 _imagingSettings.clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);

    if (_renderer && GetCurrentStage()) {
        // Render hydra
        // Set camera and lighting state
        _imagingSettings.SetLightPositionFromCamera(GetCurrentCamera());
        _renderer->SetLightingState(_imagingSettings.GetLights(), _imagingSettings._material, _imagingSettings._ambient);

        // Clipping planes
        _imagingSettings.clipPlanes.clear();
        for (int i = 0; i < GetCurrentCamera().GetClippingPlanes().size(); ++i) {
            _imagingSettings.clipPlanes.emplace_back(GetCurrentCamera().GetClippingPlanes()[i]); // convert float to double
        }

        GfVec4d viewport(0, 0, width, height);
        GfRect2i renderBufferRect(GfVec2i(0, 0), width, height);
        GfRange2f displayWindow(GfVec2f(viewport[0], height - viewport[1] - viewport[3]),
                                GfVec2f(viewport[0] + viewport[2], height - viewport[1]));
        GfRect2i dataWindow = renderBufferRect.GetIntersection(
            GfRect2i(GfVec2i(viewport[0], height - viewport[1] - viewport[3]), viewport[2], viewport[3]));
        CameraUtilFraming framing(displayWindow, dataWindow);
        _renderer->SetRenderBufferSize(renderSize);
        _renderer->SetFraming(framing);
#if PXR_VERSION <= 2311
        _renderer->SetOverrideWindowPolicy(std::make_pair(true, CameraUtilConformWindowPolicy::CameraUtilMatchHorizontally));
#else
        _renderer->SetOverrideWindowPolicy(std::make_optional(CameraUtilConformWindowPolicy::CameraUtilMatchHorizontally));
#endif
        // As of today, camera used for SetCameraPath are similar to GetViewportCamera.
        // This might change in the future and that could cause an issue for the computation
        // of the manipulator positions.
        //       if (_cameras.IsUsingStageCamera()) {
        //            _renderer->SetCameraPath(_cameras.GetStageCameraPath());
        //        } else {
        const GfCamera viewportCamera = GetViewportCamera(width, height);
        _renderer->SetCameraState(viewportCamera.GetFrustum().ComputeViewMatrix(),
                                  viewportCamera.GetFrustum().ComputeProjectionMatrix());
        //      }
        _renderer->Render(GetCurrentStage()->GetPseudoRoot(), _imagingSettings);
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // Draw grid. TODO: this should be in a usd render task
    // TODO the grid should handle the ortho case
    if (_imagingSettings.showGrid) {
        _grid.Render(*this);
    }
    if (_imagingSettings.showGizmos) {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    _drawTarget->Unbind();
}

void Viewport::SetCurrentTimeCode(const UsdTimeCode &tc) { _imagingSettings.frame = tc; }

/// Update anything that could have change after a frame render
void Viewport::Update() {
    if (GetCurrentStage()) {
        bool firstTimeStageLoaded = false;
        if (_renderStage != GetCurrentStage()) {
            firstTimeStageLoaded = true;
            SdfPathVector excludedPaths;

            // must clear first to release physics engine
            _renderer = nullptr;
            _renderer = std::make_unique<runtime::RuntimeEngine>(GetCurrentStage()->GetPseudoRoot().GetPath(), excludedPaths);
            _renderStage = GetCurrentStage();

            _cameraManipulator.SetZIsUp(UsdGeomGetStageUpAxis(GetCurrentStage()) == "Z");
            _grid.SetZIsUp(UsdGeomGetStageUpAxis(GetCurrentStage()) == "Z");
            InitializeRendererAov(*_renderer);
        }

        // Update cameras state, this will assign the user selected camera for the current stage at
        // a particular time
        _cameras.Update(GetCurrentStage(), GetCurrentTimeCode());
        if (firstTimeStageLoaded) { // TODO C++20 [[unlikely]]
            // Find a camera in the stage and use it. We might want to make it optional as it slows
            // the first render
            if (!_cameras.FindAndUseStageCamera(GetCurrentStage(), GetCurrentTimeCode())) {
                // TODO: framing should probably move in the update as we want to also frame when an
                // internal ortho camera is selected
                // With the multiple viewport we might want to frame all cameras, not just the current one
                //
                FrameCameraOnRootPrim(); // TODO rename to FrameCurrentCamera
                FrameAllCameras();
            }
        }
    }

    const GfVec2i &currentSize = _drawTarget->GetSize();
    if (currentSize != _textureSize) {
        _drawTarget->Bind();
        _drawTarget->SetSize(_textureSize);
        _drawTarget->Unbind();
    }

    if (_renderer && _selection.UpdateSelectionHash(GetCurrentStage(), _lastSelectionHash)) {
        _renderer->ClearSelected();
        _renderer->SetSelected(_selection.GetSelectedPaths(GetCurrentStage()));

        // Tell the manipulators the selection has changed
        _positionManipulator.OnSelectionChange(*this);
        _rotationManipulator.OnSelectionChange(*this);
        _scaleManipulator.OnSelectionChange(*this);
    }

    if (_renderer && _imagingSettings.play) {
        _renderer->Update(1.0 / 60);
    }
}

void Viewport::SyncFabric() {
    _renderer->SyncFabric();
}

void Viewport::UnSyncFabric() {
    _renderer->UnSyncFabric();
}

void Viewport::FlushDirties() {
    _renderer->FlushDirties();
}

bool Viewport::TestIntersection(GfVec2d clickedPoint, SdfPath &outHitPrimPath, SdfPath &outHitInstancerPath,
                                int &outHitInstanceIndex) {

    GfVec2i renderSize = _drawTarget->GetSize();
    double width = static_cast<double>(renderSize[0]);
    double height = static_cast<double>(renderSize[1]);

    GfCamera viewportCamera = GetViewportCamera(width, height);
    GfFrustum pixelFrustum = viewportCamera.GetFrustum().ComputeNarrowedFrustum(clickedPoint, GfVec2d(1.0 / width, 1.0 / height));
    GfVec3d outHitPoint;
    GfVec3d outHitNormal;
    return (_renderer && GetCurrentStage() &&
            _renderer->TestIntersection(viewportCamera.GetFrustum().ComputeViewMatrix(), pixelFrustum.ComputeProjectionMatrix(),
                                        GetCurrentStage()->GetPseudoRoot(), _imagingSettings, &outHitPoint, &outHitNormal,
                                        &outHitPrimPath, &outHitInstancerPath, &outHitInstanceIndex));
}
