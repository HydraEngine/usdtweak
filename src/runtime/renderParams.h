//  Copyright (c) 2024 Feng Yang
//
//  I am making my contributions/submissions to this project solely in my
//  personal capacity and am not conveying any rights to any intellectual
//  property of any third parties.

#pragma once

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImagingGL/api.h"

#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"

namespace runtime {

enum class UsdImagingGLDrawMode {
    DRAW_POINTS,
    DRAW_WIREFRAME,
    DRAW_WIREFRAME_ON_SURFACE,
    DRAW_SHADED_FLAT,
    DRAW_SHADED_SMOOTH,
    DRAW_GEOM_ONLY,
    DRAW_GEOM_FLAT,
    DRAW_GEOM_SMOOTH
};

// Note: some assumptions are made about the order of these enums, so please
// be careful when updating them.
enum class UsdImagingGLCullStyle {
    CULL_STYLE_NO_OPINION,
    CULL_STYLE_NOTHING,
    CULL_STYLE_BACK,
    CULL_STYLE_FRONT,
    CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED,

    CULL_STYLE_COUNT
};

/// \class UsdImagingGLRenderParams
///
/// Used as an arguments class for various methods in RuntimeEngine.
///
class UsdImagingGLRenderParams {
public:
    using ClipPlanesVector = std::vector<pxr::GfVec4d>;
    using BBoxVector = std::vector<pxr::GfBBox3d>;

    pxr::UsdTimeCode frame;
    float complexity;
    UsdImagingGLDrawMode drawMode;
    bool showGuides;
    bool showProxy;
    bool showRender;
    bool forceRefresh;
    bool flipFrontFacing;
    UsdImagingGLCullStyle cullStyle;
    bool enableIdRender;
    bool enableLighting;
    bool enableSampleAlphaToCoverage;
    bool applyRenderState;
    bool gammaCorrectColors;
    bool highlight;
    pxr::GfVec4f overrideColor;
    pxr::GfVec4f wireframeColor;
    float alphaThreshold;  // threshold < 0 implies automatic
    ClipPlanesVector clipPlanes;
    bool enableSceneMaterials;
    bool enableSceneLights;
    // Respect USD's model:drawMode attribute...
    bool enableUsdDrawModes;
    pxr::GfVec4f clearColor;
    pxr::TfToken colorCorrectionMode;
    // Optional OCIO color setings, only valid when colorCorrectionMode==HdxColorCorrectionTokens->openColorIO
    int lut3dSizeOCIO;
    pxr::TfToken ocioDisplay;
    pxr::TfToken ocioView;
    pxr::TfToken ocioColorSpace;
    pxr::TfToken ocioLook;
    // BBox settings
    BBoxVector bboxes;
    pxr::GfVec4f bboxLineColor;
    float bboxLineDashSize;

    inline UsdImagingGLRenderParams();

    inline bool operator==(const UsdImagingGLRenderParams &other) const;

    inline bool operator!=(const UsdImagingGLRenderParams &other) const { return !(*this == other); }
};

UsdImagingGLRenderParams::UsdImagingGLRenderParams()
    : frame(pxr::UsdTimeCode::EarliestTime()),
      complexity(1.0),
      drawMode(UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH),
      showGuides(false),
      showProxy(true),
      showRender(false),
      forceRefresh(false),
      flipFrontFacing(false),
      cullStyle(UsdImagingGLCullStyle::CULL_STYLE_NOTHING),
      enableIdRender(false),
      enableLighting(true),
      enableSampleAlphaToCoverage(false),
      applyRenderState(true),
      gammaCorrectColors(true),
      highlight(false),
      overrideColor(.0f, .0f, .0f, .0f),
      wireframeColor(.0f, .0f, .0f, .0f),
      alphaThreshold(-1),
      clipPlanes(),
      enableSceneMaterials(true),
      enableSceneLights(true),
      enableUsdDrawModes(true),
      clearColor(0, 0, 0, 1),
      lut3dSizeOCIO(65),
      bboxLineColor(1),
      bboxLineDashSize(3) {}

bool UsdImagingGLRenderParams::operator==(const UsdImagingGLRenderParams &other) const {
    return frame == other.frame && complexity == other.complexity && drawMode == other.drawMode &&
           showGuides == other.showGuides && showProxy == other.showProxy && showRender == other.showRender &&
           forceRefresh == other.forceRefresh && flipFrontFacing == other.flipFrontFacing &&
           cullStyle == other.cullStyle && enableIdRender == other.enableIdRender &&
           enableLighting == other.enableLighting && enableSampleAlphaToCoverage == other.enableSampleAlphaToCoverage &&
           applyRenderState == other.applyRenderState && gammaCorrectColors == other.gammaCorrectColors &&
           highlight == other.highlight && overrideColor == other.overrideColor &&
           wireframeColor == other.wireframeColor && alphaThreshold == other.alphaThreshold &&
           clipPlanes == other.clipPlanes && enableSceneMaterials == other.enableSceneMaterials &&
           enableSceneLights == other.enableSceneLights && enableUsdDrawModes == other.enableUsdDrawModes &&
           clearColor == other.clearColor && colorCorrectionMode == other.colorCorrectionMode &&
           ocioDisplay == other.ocioDisplay && ocioView == other.ocioView && ocioColorSpace == other.ocioColorSpace &&
           ocioLook == other.ocioLook && lut3dSizeOCIO == other.lut3dSizeOCIO && bboxes == other.bboxes &&
           bboxLineColor == other.bboxLineColor && bboxLineDashSize == other.bboxLineDashSize;
}

}  // namespace runtime