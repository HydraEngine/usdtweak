//  Copyright (c) 2024 Feng Yang
//
//  I am making my contributions/submissions to this project solely in my
//  personal capacity and am not conveying any rights to any intellectual
//  property of any third parties.

#pragma once

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/version.h"

#include "renderParams.h"
#include "rendererSettings.h"
#include "physxEngine.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h"

#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/renderSetupTask.h"

#include "pxr/imaging/hgi/hgi.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleMaterial.h"

#include "pxr/imaging/hgi/hgi.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/tf/declarePtrs.h"

namespace PXR_INTERNAL_NS {
class UsdPrim;
class HdRenderIndex;
class HdxTaskController;
class UsdImagingDelegate;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleLightingContext);
TF_DECLARE_REF_PTRS(UsdImagingStageSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingRootOverridesSceneIndex);
TF_DECLARE_REF_PTRS(UsdImagingSelectionSceneIndex);
TF_DECLARE_REF_PTRS(HdsiLegacyDisplayStyleOverrideSceneIndex);
TF_DECLARE_REF_PTRS(HdsiPrimTypePruningSceneIndex);
TF_DECLARE_REF_PTRS(HdsiSceneGlobalsSceneIndex);
TF_DECLARE_REF_PTRS(HdSceneIndexBase);
TF_DECLARE_REF_PTRS(FabricSceneIndex);
}  // namespace PXR_INTERNAL_NS

namespace runtime {

using UsdStageWeakPtr = pxr::TfWeakPtr<class PXR_INTERNAL_NS::UsdStage>;

namespace RuntimeEngine_Impl {
using _AppSceneIndicesSharedPtr = std::shared_ptr<struct _AppSceneIndices>;
}

/// \class RuntimeEngine
///
/// The RuntimeEngine is the main entry point API for rendering USD scenes.
///
class RuntimeEngine {
public:
    /// Parameters to construct RuntimeEngine.
    struct Parameters {
        pxr::SdfPath rootPath = pxr::SdfPath::AbsoluteRootPath();
        pxr::SdfPathVector excludedPaths;
        pxr::SdfPathVector invisedPaths;
        pxr::SdfPath sceneDelegateID = pxr::SdfPath::AbsoluteRootPath();
        /// An HdDriver, containing the Hgi of your choice, can be optionally passed
        /// in during construction. This can be helpful if your application creates
        /// multiple RuntimeEngine's that wish to use the same HdDriver / Hgi.
        pxr::HdDriver driver;
        /// The \p rendererPluginId argument indicates the renderer plugin that
        /// Hydra should use. If the empty token is passed in, a default renderer
        /// plugin will be chosen depending on the value of \p gpuEnabled.
        pxr::TfToken rendererPluginId;
        /// The \p gpuEnabled argument determines if this instance will allow Hydra
        /// to use the GPU to produce images.
        bool gpuEnabled = true;
        /// \p displayUnloadedPrimsWithBounds draws bounding boxes for unloaded
        /// prims if they have extents/extentsHint authored.
        bool displayUnloadedPrimsWithBounds = false;
        /// \p allowAsynchronousSceneProcessing indicates to constructed hydra
        /// scene indices that asynchronous processing is allowow. Applications
        /// should perodically call PollForAsynchronousUpdates on the engine.
        bool allowAsynchronousSceneProcessing = false;
    };

    // ---------------------------------------------------------------------
    /// \name Construction
    /// @{
    // ---------------------------------------------------------------------

    RuntimeEngine(const Parameters& params);

    /// An HdDriver, containing the Hgi of your choice, can be optionally passed
    /// in during construction. This can be helpful if you application creates
    /// multiple RuntimeEngine that wish to use the same HdDriver / Hgi.
    /// The \p rendererPluginId argument indicates the renderer plugin that
    /// Hyrda should use. If the empty token is passed in, a default renderer
    /// plugin will be chosen depending on the value of \p gpuEnabled.
    /// The \p gpuEnabled argument determines if this instance will allow Hydra
    /// to use the GPU to produce images.
    RuntimeEngine(const pxr::HdDriver& driver = pxr::HdDriver(),
                  const pxr::TfToken& rendererPluginId = pxr::TfToken(),
                  bool gpuEnabled = true);

    RuntimeEngine(const pxr::SdfPath& rootPath,
                  const pxr::SdfPathVector& excludedPaths,
                  const pxr::SdfPathVector& invisedPaths = pxr::SdfPathVector(),
                  const pxr::SdfPath& sceneDelegateID = pxr::SdfPath::AbsoluteRootPath(),
                  const pxr::HdDriver& driver = pxr::HdDriver(),
                  const pxr::TfToken& rendererPluginId = pxr::TfToken(),
                  bool gpuEnabled = true,
                  bool displayUnloadedPrimsWithBounds = false,
                  bool allowAsynchronousSceneProcessing = false);

    // Disallow copies
    RuntimeEngine(const RuntimeEngine&) = delete;
    RuntimeEngine& operator=(const RuntimeEngine&) = delete;

    ~RuntimeEngine();

    /// @}

    // ---------------------------------------------------------------------
    /// \name Rendering
    /// @{
    // ---------------------------------------------------------------------
    void Update(float dt);

    void FlushDirties();

    void SyncFabric();

    void UnSyncFabric();

    /// Support for batched drawing
    void PrepareBatch(const pxr::UsdPrim& root, const UsdImagingGLRenderParams& params);

    void RenderBatch(const pxr::SdfPathVector& paths, const UsdImagingGLRenderParams& params);

    /// Entry point for kicking off a render
    void Render(const pxr::UsdPrim& root, const UsdImagingGLRenderParams& params);

    /// Returns true if the resulting image is fully converged.
    /// (otherwise, caller may need to call Render() again to refine the result)
    bool IsConverged() const;

    /// @}

    // ---------------------------------------------------------------------
    /// \name Root Transform and Visibility
    /// @{
    // ---------------------------------------------------------------------

    /// Sets the root transform.
    void SetRootTransform(pxr::GfMatrix4d const& xf);

    /// Sets the root visibility.
    void SetRootVisibility(bool isVisible);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Camera State
    /// @{
    // ---------------------------------------------------------------------

    /// Scene camera API
    /// Set the scene camera path to use for rendering.
    void SetCameraPath(pxr::SdfPath const& id);

    /// Determines how the filmback of the camera is mapped into
    /// the pixels of the render buffer and what pixels of the render
    /// buffer will be rendered into.
    void SetFraming(pxr::CameraUtilFraming const& framing);

    /// Specifies whether to force a window policy when conforming
    /// the frustum of the camera to match the display window of
    /// the camera framing.
    ///
    /// If set to {false, ...}, the window policy of the specified camera
    /// will be used.
    ///
    /// Note: std::pair<bool, ...> is used instead of std::optional<...>
    /// because the latter is only available in C++17 or later.
    void SetOverrideWindowPolicy(const std::optional<pxr::CameraUtilConformWindowPolicy>& policy);

    /// Set the size of the render buffers baking the AOVs.
    /// GUI applications should set this to the size of the window.
    ///
    void SetRenderBufferSize(pxr::GfVec2i const& size);

    /// Set the viewport to use for rendering as (x,y,w,h), where (x,y)
    /// represents the lower left corner of the viewport rectangle, and (w,h)
    /// is the width and height of the viewport in pixels.
    ///
    /// \deprecated Use SetFraming and SetRenderBufferSize instead.
    void SetRenderViewport(pxr::GfVec4d const& viewport);

    /// Set the window policy to use.
    /// XXX: This is currently used for scene cameras set via SetCameraPath.
    /// See comment in SetCameraState for the free cam.
    void SetWindowPolicy(pxr::CameraUtilConformWindowPolicy policy);

    /// Free camera API
    /// Set camera framing state directly (without pointing to a camera on the
    /// USD stage). The projection matrix is expected to be pre-adjusted for the
    /// window policy.
    void SetCameraState(const pxr::GfMatrix4d& viewMatrix, const pxr::GfMatrix4d& projectionMatrix);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Light State
    /// @{
    // ---------------------------------------------------------------------

    /// Copy lighting state from another lighting context.
    void SetLightingState(pxr::GlfSimpleLightingContextPtr const& src);

    /// Set lighting state
    /// Derived classes should ensure that passing an empty lights
    /// vector disables lighting.
    /// \param lights is the set of lights to use, or empty to disable lighting.
    void SetLightingState(pxr::GlfSimpleLightVector const& lights,
                          pxr::GlfSimpleMaterial const& material,
                          pxr::GfVec4f const& sceneAmbient);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Selection Highlighting
    /// @{
    // ---------------------------------------------------------------------

    /// Sets (replaces) the list of prim paths that should be included in
    /// selection highlighting. These paths may include root paths which will
    /// be expanded internally.
    void SetSelected(pxr::SdfPathVector const& paths);

    /// Clear the list of prim paths that should be included in selection
    /// highlighting.
    void ClearSelected();

    /// Add a path with instanceIndex to the list of prim paths that should be
    /// included in selection highlighting. UsdImagingDelegate::ALL_INSTANCES
    /// can be used for highlighting all instances if path is an instancer.
    void AddSelected(pxr::SdfPath const& path, int instanceIndex);

    /// Sets the selection highlighting color.
    void SetSelectionColor(pxr::GfVec4f const& color);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Picking
    /// @{
    // ---------------------------------------------------------------------

    /// Finds closest point of intersection with a frustum by rendering.
    ///
    /// This method uses a PickRender and a customized depth buffer to find an
    /// approximate point of intersection by rendering. This is less accurate
    /// than implicit methods or rendering with GL_SELECT, but leverages any
    /// data already cached in the renderer.
    ///
    /// Returns whether a hit occurred and if so, \p outHitPoint will contain
    /// the intersection point in world space (i.e. \p projectionMatrix and
    /// \p viewMatrix factored back out of the result), and \p outHitNormal
    /// will contain the world space normal at that point.
    ///
    /// \p outHitPrimPath will point to the gprim selected by the pick.
    /// \p outHitInstancerPath will point to the point instancer (if applicable)
    /// of that gprim. For nested instancing, outHitInstancerPath points to
    /// the closest instancer.
    ///
    /// \deprecated Please use the override of TestIntersection that takes
    /// PickParams and returns an IntersectionResultVector instead!
    bool TestIntersection(const pxr::GfMatrix4d& viewMatrix,
                          const pxr::GfMatrix4d& projectionMatrix,
                          const pxr::UsdPrim& root,
                          const UsdImagingGLRenderParams& params,
                          pxr::GfVec3d* outHitPoint,
                          pxr::GfVec3d* outHitNormal,
                          pxr::SdfPath* outHitPrimPath = nullptr,
                          pxr::SdfPath* outHitInstancerPath = nullptr,
                          int* outHitInstanceIndex = nullptr,
                          pxr::HdInstancerContext* outInstancerContext = nullptr);

    // Pick result
    struct IntersectionResult {
        pxr::GfVec3d hitPoint;
        pxr::GfVec3d hitNormal;
        pxr::SdfPath hitPrimPath;
        pxr::SdfPath hitInstancerPath;
        int hitInstanceIndex;
        pxr::HdInstancerContext instancerContext;
    };

    typedef std::vector<struct IntersectionResult> IntersectionResultVector;

    // Pick params
    struct PickParams {
        pxr::TfToken resolveMode;
    };

    /// Perform picking by finding the intersection of objects in the scene with a renderered frustum.
    /// Depending on the resolve mode it may find all objects intersecting the frustum or the closest
    /// point of intersection within the frustum.
    ///
    /// If resolve mode is set to resolveDeep it uses Deep Selection to gather all paths within
    /// the frustum even if obscured by other visible objects.
    /// If resolve mode is set to resolveNearestToCenter it uses a PickRender and
    /// a customized depth buffer to find all approximate points of intersection by rendering.
    /// This is less accurate than implicit methods or rendering with GL_SELECT, but leverages any
    /// data already cached in the renderer.
    ///
    /// Returns whether a hit occurred and if so, \p outResults will point to all the
    /// gprims selected by the pick as determined by the resolve mode.
    /// \p outHitPoint will contain the intersection point in world space
    /// (i.e. \p projectionMatrix and \p viewMatrix factored back out of the result)
    /// \p outHitNormal will contain the world space normal at that point.
    /// \p hitPrimPath will point to the gprim selected by the pick.
    /// \p hitInstancerPath will point to the point instancer (if applicable) of each gprim.
    ///
    bool TestIntersection(const PickParams& pickParams,
                          const pxr::GfMatrix4d& viewMatrix,
                          const pxr::GfMatrix4d& projectionMatrix,
                          const pxr::UsdPrim& root,
                          const UsdImagingGLRenderParams& params,
                          IntersectionResultVector* outResults);

    /// Decodes a pick result given hydra prim ID/instance ID (like you'd get
    /// from an ID render).
    bool DecodeIntersection(unsigned char const primIdColor[4],
                            unsigned char const instanceIdColor[4],
                            pxr::SdfPath* outHitPrimPath = nullptr,
                            pxr::SdfPath* outHitInstancerPath = nullptr,
                            int* outHitInstanceIndex = nullptr,
                            pxr::HdInstancerContext* outInstancerContext = nullptr);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Renderer Plugin Management
    /// @{
    // ---------------------------------------------------------------------

    /// Return the vector of available render-graph delegate plugins.
    static pxr::TfTokenVector GetRendererPlugins();

    /// Return the user-friendly description of a renderer plugin.
    static std::string GetRendererDisplayName(pxr::TfToken const& id);

    /// Return if the GPU is enabled and can be used for any rendering tasks.
    bool GetGPUEnabled() const;

    /// Return the id of the currently used renderer plugin.
    pxr::TfToken GetCurrentRendererId() const;

    /// Set the current render-graph delegate to \p id.
    /// the plugin will be loaded if it's not yet.
    bool SetRendererPlugin(pxr::TfToken const& id);

    /// @}

    // ---------------------------------------------------------------------
    /// \name AOVs
    /// @{
    // ---------------------------------------------------------------------

    /// Return the vector of available renderer AOV settings.
    pxr::TfTokenVector GetRendererAovs() const;

    /// Set the current renderer AOV to \p id.
    bool SetRendererAov(pxr::TfToken const& id);

    /// Returns an AOV texture handle for the given token.
    pxr::HgiTextureHandle GetAovTexture(pxr::TfToken const& name) const;

    /// Returns the AOV render buffer for the given token.
    pxr::HdRenderBuffer* GetAovRenderBuffer(pxr::TfToken const& name) const;

    // ---------------------------------------------------------------------
    /// \name Render Settings (Legacy)
    /// @{
    // ---------------------------------------------------------------------

    /// Returns the list of renderer settings.
    UsdImagingGLRendererSettingsList GetRendererSettingsList() const;

    /// Gets a renderer setting's current value.
    pxr::VtValue GetRendererSetting(pxr::TfToken const& id) const;

    /// Sets a renderer setting's value.
    void SetRendererSetting(pxr::TfToken const& id, pxr::VtValue const& value);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Scene-defined Render Pass and Render Settings
    /// \note Support is WIP.
    /// @{
    // ---------------------------------------------------------------------

    /// Set active render pass prim to use to drive rendering.
    void SetActiveRenderPassPrimPath(pxr::SdfPath const&);

    /// Set active render settings prim to use to drive rendering.
    void SetActiveRenderSettingsPrimPath(pxr::SdfPath const&);

    /// Utility method to query available render settings prims.
    static pxr::SdfPathVector GetAvailableRenderSettingsPrimPaths(pxr::UsdPrim const& root);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Presentation
    /// @{
    // ---------------------------------------------------------------------

    /// Enable / disable presenting the render to bound framebuffer.
    /// An application may choose to manage the AOVs that are rendered into
    /// itself and skip the engine's presentation.
    void SetEnablePresentation(bool enabled);

    /// The destination API (e.g., OpenGL, see hgiInterop for details) and
    /// framebuffer that the AOVs are presented into. The framebuffer
    /// is a VtValue that encoding a framebuffer in a destination API
    /// specific way.
    /// E.g., a uint32_t (aka GLuint) for framebuffer object for OpenGL.
    void SetPresentationOutput(pxr::TfToken const& api, pxr::VtValue const& framebuffer);

    /// @}

    // ---------------------------------------------------------------------
    /// \name Renderer Command API
    /// @{
    // ---------------------------------------------------------------------

    /// Return command deescriptors for commands supported by the active
    /// render delegate.
    ///
    pxr::HdCommandDescriptors GetRendererCommandDescriptors() const;

    /// Invokes command on the active render delegate. If successful, returns
    /// \c true, returns \c false otherwise. Note that the command will not
    /// succeeed if it is not among those returned by
    /// GetRendererCommandDescriptors() for the same active render delegate.
    ///
    bool InvokeRendererCommand(const pxr::TfToken& command,
                               const pxr::HdCommandArgs& args = pxr::HdCommandArgs()) const;

    // ---------------------------------------------------------------------
    /// \name Control of background rendering threads.
    /// @{
    // ---------------------------------------------------------------------

    /// Query the renderer as to whether it supports pausing and resuming.
    bool IsPauseRendererSupported() const;

    /// Pause the renderer.
    ///
    /// Returns \c true if successful.
    bool PauseRenderer();

    /// Resume the renderer.
    ///
    /// Returns \c true if successful.
    bool ResumeRenderer();

    /// Query the renderer as to whether it supports stopping and restarting.
    bool IsStopRendererSupported() const;

    /// Stop the renderer.
    ///
    /// Returns \c true if successful.
    bool StopRenderer();

    /// Restart the renderer.
    ///
    /// Returns \c true if successful.
    bool RestartRenderer();

    /// @}

    // ---------------------------------------------------------------------
    /// \name Color Correction
    /// @{
    // ---------------------------------------------------------------------

    /// Set \p ccType to one of the HdxColorCorrectionTokens:
    /// {disabled, sRGB, openColorIO}
    ///
    /// If 'openColorIO' is used, \p ocioDisplay, \p ocioView, \p ocioColorSpace
    /// and \p ocioLook are options the client may supply to configure OCIO.
    /// \p ocioColorSpace refers to the input (source) color space.
    /// The default value is substituted if an option isn't specified.
    /// You can find the values for these strings inside the
    /// profile/config .ocio file. For example:
    ///
    ///  displays:
    ///    rec709g22:
    ///      !<View> {name: studio, colorspace: linear, looks: studio_65_lg2}
    ///
    void SetColorCorrectionSettings(pxr::TfToken const& ccType,
                                    pxr::TfToken const& ocioDisplay = {},
                                    pxr::TfToken const& ocioView = {},
                                    pxr::TfToken const& ocioColorSpace = {},
                                    pxr::TfToken const& ocioLook = {});

    /// @}

    /// Returns true if the platform is color correction capable.
    static bool IsColorCorrectionCapable();

    // ---------------------------------------------------------------------
    /// \name Render Statistics
    /// @{
    // ---------------------------------------------------------------------

    /// Returns render statistics.
    ///
    /// The contents of the dictionary will depend on the current render
    /// delegate.
    ///
    pxr::VtDictionary GetRenderStats() const;

    /// @}

    // ---------------------------------------------------------------------
    /// \name HGI
    /// @{
    // ---------------------------------------------------------------------

    /// Returns the HGI interface.
    ///
    pxr::Hgi* GetHgi();

    /// @}

    // ---------------------------------------------------------------------
    /// \name Asynchronous
    /// @{
    // ---------------------------------------------------------------------

    /// If \p allowAsynchronousSceneProcessing is true within the Parameters
    /// provided to the RuntimeEngine constructor, an application can
    /// periodically call this from the main thread.
    ///
    /// A return value of true indicates that the scene has changed and the
    /// render should be updated.
    bool PollForAsynchronousUpdates() const;

    /// @}

protected:
    /// Open some protected methods for whitebox testing.
    friend class UsdImagingGL_UnitTestGLDrawing;

    /// Returns the render index of the engine, if any.  This is only used for
    /// whitebox testing.
    pxr::HdRenderIndex* _GetRenderIndex() const;

    void _Execute(const UsdImagingGLRenderParams& params, pxr::HdTaskSharedPtrVector tasks);

    bool _CanPrepare(const pxr::UsdPrim& root);

    void _PreSetTime(const UsdImagingGLRenderParams& params);

    void _PostSetTime(const UsdImagingGLRenderParams& params);

    void _PrepareRender(const UsdImagingGLRenderParams& params);

    void _SetActiveRenderSettingsPrimFromStageMetadata(pxr::UsdStageWeakPtr stage);

    void _SetSceneGlobalsCurrentFrame(pxr::UsdTimeCode const& time);

    void _UpdateDomeLightCameraVisibility();

    using BBoxVector = std::vector<pxr::GfBBox3d>;

    void _SetBBoxParams(const BBoxVector& bboxes, const pxr::GfVec4f& bboxLineColor, float bboxLineDashSize);

    // Create a hydra collection given root paths and render params.
    // Returns true if the collection was updated.
    static bool _UpdateHydraCollection(pxr::HdRprimCollection* collection,
                                       pxr::SdfPathVector const& roots,
                                       UsdImagingGLRenderParams const& params);

    static pxr::HdxRenderTaskParams _MakeHydraUsdImagingGLRenderParams(UsdImagingGLRenderParams const& params);

    static void _ComputeRenderTags(UsdImagingGLRenderParams const& params, pxr::TfTokenVector* renderTags);

    void _InitializeHgiIfNecessary();

    void _SetRenderDelegateAndRestoreState(pxr::HdPluginRenderDelegateUniqueHandle&&);

    void _SetRenderDelegate(pxr::HdPluginRenderDelegateUniqueHandle&&);

    pxr::SdfPath _ComputeControllerPath(const pxr::HdPluginRenderDelegateUniqueHandle&);

    static pxr::TfToken _GetDefaultRendererPluginId();

    pxr::HdEngine* _GetHdEngine();

    pxr::HdxTaskController* _GetTaskController() const;

    pxr::HdSelectionSharedPtr _GetSelection() const;

protected:
    // Note that any of the fields below might become private
    // in the future and subclasses should use the above getters
    // to access them instead.

    pxr::HgiUniquePtr _hgi;
    // Similar for HdDriver.
    pxr::HdDriver _hgiDriver;

    pxr::VtValue _userFramebuffer;

protected:
    bool _displayUnloadedPrimsWithBounds;
    bool _gpuEnabled;
    pxr::HdPluginRenderDelegateUniqueHandle _renderDelegate;
    std::unique_ptr<pxr::HdRenderIndex> _renderIndex;

    pxr::SdfPath const _sceneDelegateId;

    std::unique_ptr<pxr::HdxTaskController> _taskController;

    pxr::HdxSelectionTrackerSharedPtr _selTracker;
    pxr::HdRprimCollection _renderCollection;
    pxr::HdRprimCollection _intersectCollection;

    pxr::GlfSimpleLightingContextRefPtr _lightingContextForOpenGLState;

    // Data we want to live across render plugin switches:
    pxr::GfVec4f _selectionColor;
    bool _domeLightCameraVisibility;

    pxr::SdfPath _rootPath;
    pxr::SdfPathVector _excludedPrimPaths;
    pxr::SdfPathVector _invisedPrimPaths;
    bool _isPopulated;

private:
    // Registers app-managed scene indices with the scene index plugin registry.
    // This needs to be called once *before* the render index is constructed.
    static void _RegisterApplicationSceneIndices();

    // Creates and returns the scene globals scene index. This callback is
    // registered prior to render index construction and is invoked during
    // render index construction via
    // HdSceneIndexPluginRegistry::AppendSceneIndicesForRenderer(..).
    static pxr::HdSceneIndexBaseRefPtr _AppendSceneGlobalsSceneIndexCallback(
            const std::string& renderInstanceId,
            const pxr::HdSceneIndexBaseRefPtr& inputScene,
            const pxr::HdContainerDataSourceHandle& inputArgs);

    pxr::HdSceneIndexBaseRefPtr _AppendOverridesSceneIndices(const pxr::HdSceneIndexBaseRefPtr& inputScene);

    RuntimeEngine_Impl::_AppSceneIndicesSharedPtr _appSceneIndices;

    void _DestroyHydraObjects();

    // Note that we'll only ever use one of _sceneIndex/_sceneDelegate
    // at a time.
    pxr::UsdImagingStageSceneIndexRefPtr _stageSceneIndex;
    pxr::UsdImagingSelectionSceneIndexRefPtr _selectionSceneIndex;
    pxr::UsdImagingRootOverridesSceneIndexRefPtr _rootOverridesSceneIndex;
    pxr::HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr _displayStyleSceneIndex;
    pxr::HdsiPrimTypePruningSceneIndexRefPtr _materialPruningSceneIndex;
    pxr::HdsiPrimTypePruningSceneIndexRefPtr _lightPruningSceneIndex;
    pxr::HdSceneIndexBaseRefPtr _sceneIndex;
    pxr::FabricSceneIndexRefPtr _fabricSceneIndex;
    std::unique_ptr<sim::PhysxEngine> _simulationEngine;

    std::unique_ptr<pxr::UsdImagingDelegate> _sceneDelegate;

    std::unique_ptr<pxr::HdEngine> _engine;

    bool _allowAsynchronousSceneProcessing = false;
};

}  // namespace runtime
