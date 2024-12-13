//  Copyright (c) 2024 Feng Yang
//
//  I am making my contributions/submissions to this project solely in my
//  personal capacity and am not conveying any rights to any intellectual
//  property of any third parties.

#pragma once

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/pxr.h"

namespace runtime {

struct UsdImagingGLRendererSetting {
    enum Type { TYPE_FLAG, TYPE_INT, TYPE_FLOAT, TYPE_STRING };
    std::string name;
    pxr::TfToken key;
    Type type;
    pxr::VtValue defValue;
};

typedef std::vector<UsdImagingGLRendererSetting> UsdImagingGLRendererSettingsList;

}  // namespace runtime