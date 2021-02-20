#include <iostream>
#include <pxr/base/gf/line.h>
#include "Constants.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "RotationManipulator.h"
#include "GeometricFunctions.h"
#include "Viewport.h"
#include "Gui.h"
#include "Commands.h"

static constexpr GLfloat axisSize = 1.2f;

static constexpr const GLfloat planesPoints[] = {
    // The 3 quads on planes yz, xz, xy
    0.f,       -axisSize, axisSize, 0.f,      axisSize,  axisSize,  0.f,       -axisSize, -axisSize,
    0.f,       axisSize,  axisSize, 0.f,      axisSize,  -axisSize, 0.f,       -axisSize, -axisSize,

    -axisSize, 0.f,       axisSize, axisSize, 0.f,       axisSize,  -axisSize, 0.f,       -axisSize,
    axisSize,  0.f,       axisSize, axisSize, 0.f,       -axisSize, -axisSize, 0.f,       -axisSize,

    -axisSize, axisSize,  0.f,      axisSize, axisSize,  0.f,       -axisSize, -axisSize, 0.f,
    axisSize,  axisSize,  0.f,      axisSize, -axisSize, 0.f,       -axisSize, -axisSize, 0.f};

static constexpr const GLfloat planesUV[] = {
    -axisSize, axisSize, axisSize, axisSize, -axisSize, -axisSize, axisSize, axisSize, axisSize, -axisSize, -axisSize, -axisSize,

    -axisSize, axisSize, axisSize, axisSize, -axisSize, -axisSize, axisSize, axisSize, axisSize, -axisSize, -axisSize, -axisSize,

    -axisSize, axisSize, axisSize, axisSize, -axisSize, -axisSize, axisSize, axisSize, axisSize, -axisSize, -axisSize, -axisSize};

static constexpr const GLfloat planesColor[] = {
    1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f,

    0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f,

    0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 1.f};

static constexpr const char *vertexShaderSrc = "#version 330 core\n"
                                               "layout (location = 0) in vec3 aPos;"
                                               "layout (location = 1) in vec2 inUv;"
                                               "layout (location = 2) in vec3 inColor;"
                                               "uniform vec3 scale;"
                                               "uniform mat4 modelView;"
                                               "uniform mat4 projection;"
                                               "uniform mat4 objectMatrix;"
                                               "out vec4 color;"
                                               "out vec2 uv;"
                                               "void main()"
                                               "{"
                                               "    vec4 bPos = objectMatrix*vec4(aPos*scale, 1.0);"
                                               "    gl_Position = projection*modelView*bPos;"
                                               "    color = vec4(inColor.rgb, 1.0);"
                                               "    uv = inUv;"
                                               "}";

// First approach was to draw circles on a quad using fragment shader, fun, but not the best solution
// TODO: change to drawing lines as this really doesn't look nice
static constexpr const char *fragmentShaderSrc =
    "#version 330 core\n"
    "in vec4 color;"
    "in vec2 uv;"
    "uniform vec3 highlight;"
    "out vec4 FragColor;"
    "void main()"
    "{"
    "    float alpha = abs(1.f-sqrt(uv.x*uv.x+uv.y*uv.y));"
    "    float derivative = length(fwidth(uv));"
    "    if (derivative > 0.1) discard;"
    "    alpha = smoothstep(2.0*derivative, 0.0, alpha);"
    "    if (alpha < 0.2) discard;"
    "    if (dot(highlight,color.xyz) >0.9) { FragColor = vec4(1.0, 1.0, 0.2, alpha);}"
    "    else {FragColor = vec4(color.xyz, alpha);}"
    "}";

RotationManipulator::RotationManipulator() : _selectedAxis(None) {

    // Vertex array object
    glGenVertexArrays(1, &_vertexArrayObject);
    glBindVertexArray(_vertexArrayObject);

    if (CompileShaders()) {
        _scaleFactorUniform = glGetUniformLocation(_programShader, "scale");
        _modelViewUniform = glGetUniformLocation(_programShader, "modelView");
        _projectionUniform = glGetUniformLocation(_programShader, "projection");
        _objectMatrixUniform = glGetUniformLocation(_programShader, "objectMatrix");
        _highlightUniform = glGetUniformLocation(_programShader, "highlight");

        // We store points and colors in the same buffer
        glGenBuffers(1, &_arrayBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, _arrayBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planesPoints) + sizeof(planesUV) + sizeof(planesColor), nullptr, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(planesPoints), planesPoints);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(planesPoints), sizeof(planesUV), planesUV);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(planesPoints) + sizeof(planesUV), sizeof(planesColor), planesColor);

        GLint vertexAttr = glGetAttribLocation(_programShader, "aPos");
        glVertexAttribPointer(vertexAttr, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)0);
        glEnableVertexAttribArray(vertexAttr);

        GLint uvAttr = glGetAttribLocation(_programShader, "inUv");
        glVertexAttribPointer(uvAttr, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)sizeof(planesPoints));
        glEnableVertexAttribArray(uvAttr);

        GLint colorAttr = glGetAttribLocation(_programShader, "inColor");
        glVertexAttribPointer(colorAttr, 3, GL_FLOAT, GL_TRUE, 0, (GLvoid *)(sizeof(planesPoints) + sizeof(planesUV)));
        glEnableVertexAttribArray(colorAttr);

        // glBindBuffer(GL_ARRAY_BUFFER, 0);
    } else {
        exit(ERROR_UNABLE_TO_COMPILE_SHADER);
    }

    glBindVertexArray(0);
}

RotationManipulator::~RotationManipulator() { glDeleteBuffers(1, &_arrayBuffer); }

bool RotationManipulator::CompileShaders() {

    // Associate shader source with shader id
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);

    // Compile shaders
    int success = 0;
    constexpr size_t logSize = 512;
    char logStr[logSize];
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, logSize, nullptr, logStr);
        std::cout << "Compilation failed\n" << logStr << std::endl;
        return false;
    }
    glCompileShader(fragmentShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, logSize, nullptr, logStr);
        std::cout << "Compilation failed\n" << logStr << std::endl;
        return false;
    }
    _programShader = glCreateProgram();

    glAttachShader(_programShader, vertexShader);
    glAttachShader(_programShader, fragmentShader);
    glBindAttribLocation(_programShader, 0, "aPos");
    glBindAttribLocation(_programShader, 1, "inUv");
    glBindAttribLocation(_programShader, 2, "inColor");
    glLinkProgram(_programShader);

    glGetProgramiv(_programShader, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(_programShader, logSize, nullptr, logStr);
        std::cout << "Program linking failed\n" << logStr << std::endl;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
}

static bool IntersectsUnitCircle(const GfVec3d &planeNormal3d, const GfVec3d &planeOrigin3d, const GfRay &ray, float scale) {
    if (scale == 0)
        return false;

    GfPlane plane(planeNormal3d, planeOrigin3d);
    double distance = 0.0;
    constexpr float limit = 0.1; // TODO: this should be computed relative to the apparent width of the circle lines
    if (ray.Intersect(plane, &distance) && distance > 0.0) {
        const auto intersection = ray.GetPoint(distance);
        const auto segment = planeOrigin3d - intersection;

        if (fabs(1.f - segment.GetLength() / scale) < limit) {
            return true;
        }
    }
    return false;
}

bool RotationManipulator::IsMouseOver(const Viewport &viewport) {

    if (_xformAPI) {
        const GfVec2d mousePosition = viewport.GetMousePosition();
        const auto &frustum = viewport.GetCurrentCamera().GetFrustum();
        const GfRay ray = frustum.ComputeRay(mousePosition);
        const auto manipulatorCoordinates = ComputeManipulatorToWorldTransform(viewport);
        const GfVec3d xAxis = manipulatorCoordinates.GetRow3(0);
        const GfVec3d yAxis = manipulatorCoordinates.GetRow3(1);
        const GfVec3d zAxis = manipulatorCoordinates.GetRow3(2);
        const GfVec3d origin = manipulatorCoordinates.GetRow3(3);

        // Circles are scaled to keep the same screen size
        double scale = viewport.ComputeScaleFactor(origin, axisSize);

        if (IntersectsUnitCircle(xAxis, origin, ray, scale)) {
            _selectedAxis = XAxis;
            return true;
        } else if (IntersectsUnitCircle(yAxis, origin, ray, scale)) {
            _selectedAxis = YAxis;
            return true;
        } else if (IntersectsUnitCircle(zAxis, origin, ray, scale)) {
            _selectedAxis = ZAxis;
            return true;
        }
    }
    _selectedAxis = None;
    return false;
}

void RotationManipulator::OnSelectionChange(Viewport &viewport) {
    // TODO: we should set here if the new selection will be editable or not
    auto &selection = viewport.GetSelection();
    auto primPath = GetSelectedPath(selection);
    _xformAPI = UsdGeomXformCommonAPI(viewport.GetCurrentStage()->GetPrimAtPath(primPath));
}

GfMatrix4d RotationManipulator::ComputeManipulatorToWorldTransform(const Viewport &viewport) {
    if (_xformAPI) {
        const auto currentTime = GetViewportTimeCode(viewport);
        GfVec3d translation;
        GfVec3f rotation;
        GfVec3f scale;
        GfVec3f pivot;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;
        _xformAPI.GetXformVectors(&translation, &rotation, &scale, &pivot, &rotOrder, currentTime);

        const GfMatrix4d rotMat =
            UsdGeomXformOp::GetOpTransform(UsdGeomXformCommonAPI::ConvertRotationOrderToOpType(rotOrder), VtValue(rotation));

        const auto transMat = GfMatrix4d(1.0).SetTranslate(translation);
        const auto pivotMat = GfMatrix4d(1.0).SetTranslate(pivot);
        const auto xformable = UsdGeomXformable(_xformAPI.GetPrim());
        const auto parentToWorldMat = xformable.ComputeParentToWorldTransform(currentTime);

        // We are just interested in the pivot position and the orientation
        const GfMatrix4d toManipulator = rotMat * pivotMat * transMat * parentToWorldMat;

        return toManipulator.GetOrthonormalized();
    }
    return GfMatrix4d(1.0);
}

void RotationManipulator::OnDrawFrame(const Viewport &viewport) {

    if (_xformAPI) {
        const auto &camera = viewport.GetCurrentCamera();
        auto mv = camera.GetFrustum().ComputeViewMatrix();
        auto proj = camera.GetFrustum().ComputeProjectionMatrix();
        auto manipulatorCoordinates = ComputeManipulatorToWorldTransform(viewport);
        auto origin = manipulatorCoordinates.ExtractTranslation();

        // Circles are scaled to keep the same screen size
        double scale = viewport.ComputeScaleFactor(origin, axisSize);

        float toWorldf[16];
        for (int i = 0; i < 16; ++i) {
            toWorldf[i] = static_cast<float>(manipulatorCoordinates.data()[i]);
        }

        // glEnable(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST); // TESTING
        glUseProgram(_programShader);
        GfMatrix4f mvp(mv);
        GfMatrix4f projp(proj);
        glUniformMatrix4fv(_modelViewUniform, 1, GL_FALSE, mvp.data());
        glUniformMatrix4fv(_projectionUniform, 1, GL_FALSE, projp.data());
        glUniformMatrix4fv(_objectMatrixUniform, 1, GL_FALSE, toWorldf);
        glUniform3f(_scaleFactorUniform, scale, scale, scale);
        if (_selectedAxis == XAxis)
            glUniform3f(_highlightUniform, 1.0, 0.0, 0.0);
        else if (_selectedAxis == YAxis)
            glUniform3f(_highlightUniform, 0.0, 1.0, 0.0);
        else if (_selectedAxis == ZAxis)
            glUniform3f(_highlightUniform, 0.0, 0.0, 1.0);
        else
            glUniform3f(_highlightUniform, 0.0, 0.0, 0.0);
        glBindVertexArray(_vertexArrayObject);
        glDrawArrays(GL_TRIANGLES, 0, 18);
        glBindVertexArray(0);
        // glDisable(GL_DEPTH_TEST);
    }
}

// TODO: find a more meaningful function name
GfVec3d RotationManipulator::ComputeClockHandVector(Viewport &viewport) {

    const GfPlane plane(_planeNormal3d, _planeOrigin3d);
    double distance = 0.0;

    const GfVec2d mousePosition = viewport.GetMousePosition();
    const GfFrustum frustum = viewport.GetCurrentCamera().GetFrustum();
    const GfRay ray = frustum.ComputeRay(mousePosition);
    if (ray.Intersect(plane, &distance)) {
        const auto intersection = ray.GetPoint(distance);
        return _planeOrigin3d - intersection;
    }

    return GfVec3d();
}

void RotationManipulator::OnBeginEdition(Viewport &viewport) {
    if (_xformAPI) {

        const auto manipulatorCoordinates = ComputeManipulatorToWorldTransform(viewport);
        _planeOrigin3d = manipulatorCoordinates.ExtractTranslation();
        _planeNormal3d = GfVec3d(); // default init
        if (_selectedAxis == XAxis) {
            _planeNormal3d = manipulatorCoordinates.GetRow3(0);
        } else if (_selectedAxis == YAxis) {
            _planeNormal3d = manipulatorCoordinates.GetRow3(1);
        } else if (_selectedAxis == ZAxis) {
            _planeNormal3d = manipulatorCoordinates.GetRow3(2);
        }

        // Compute rotation starting point
        _rotateFrom = ComputeClockHandVector(viewport);

        // Save the rotation values
        GfVec3d translation;
        GfVec3f rotation;
        GfVec3f scale;
        GfVec3f pivot;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;
        _xformAPI.GetXformVectors(&translation, &rotation, &scale, &pivot, &rotOrder, GetViewportTimeCode(viewport));

        _rotateMatrixOnBegin =
            UsdGeomXformOp::GetOpTransform(UsdGeomXformCommonAPI::ConvertRotationOrderToOpType(rotOrder), VtValue(rotation));
        ;
    }
    BeginEdition(viewport.GetCurrentStage());
}

Manipulator *RotationManipulator::OnUpdate(Viewport &viewport) {
    if (ImGui::IsMouseReleased(0)) {
        return viewport.GetManipulator<MouseHoverManipulator>();
    }
    if (_xformAPI && _selectedAxis != None) {

        // Compute rotation angle in world coordinates
        const GfVec3d rotateTo = ComputeClockHandVector(viewport);
        const GfRotation worldRotation(_rotateFrom, rotateTo);
        const auto axisSign = _planeNormal3d * worldRotation.GetAxis() > 0 ? 1.0 : -1.0;

        // Compute rotation axis in local coordinates
        // We use the plane normal as the rotation between _rotateFrom and rotateTo might not land exactly on the rotation axis
        const GfVec3d xAxis = _rotateMatrixOnBegin.GetRow3(0);
        const GfVec3d yAxis = _rotateMatrixOnBegin.GetRow3(1);
        const GfVec3d zAxis = _rotateMatrixOnBegin.GetRow3(2);

        GfVec3d localPlaneNormal = xAxis; // default init
        if (_selectedAxis == XAxis) {
            localPlaneNormal = xAxis;
        } else if (_selectedAxis == YAxis) {
            localPlaneNormal = yAxis;
        } else if (_selectedAxis == ZAxis) {
            localPlaneNormal = zAxis;
        }

        const GfRotation deltaRotation(localPlaneNormal * axisSign, worldRotation.GetAngle());

        const GfMatrix4d resultingRotation = GfMatrix4d(1.0).SetRotate(deltaRotation) * _rotateMatrixOnBegin;

        // Get latest rotation values to give a hint to the decompose function
        GfVec3d translation;
        GfVec3f rotation;
        GfVec3f scale;
        GfVec3f pivot;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;
        _xformAPI.GetXformVectors(&translation, &rotation, &scale, &pivot, &rotOrder, GetViewportTimeCode(viewport));
        double thetaTw = GfDegreesToRadians(rotation[0]);
        double thetaFB = GfDegreesToRadians(rotation[1]);
        double thetaLR = GfDegreesToRadians(rotation[2]);
        double thetaSw = 0.0;

        // Decompose the matrix in angle values
        GfRotation::DecomposeRotation(resultingRotation, xAxis, yAxis, zAxis, 1.0, &thetaTw, &thetaFB, &thetaLR, &thetaSw, true);

        const GfVec3f newRotationValues =
            GfVec3f(GfRadiansToDegrees(thetaTw), GfRadiansToDegrees(thetaFB), GfRadiansToDegrees(thetaLR));
        _xformAPI.SetRotate(newRotationValues, rotOrder, GetEditionTimeCode(viewport));
    }

    return this;
};

void RotationManipulator::OnEndEdition(Viewport &) { EndEdition(); }

// TODO code should be shared with position manipulator
UsdTimeCode RotationManipulator::GetEditionTimeCode(const Viewport &viewport) {
    std::vector<double> timeSamples; // TODO: is there a faster way to know it the xformable has timesamples ?
    const auto xformable = UsdGeomXformable(_xformAPI.GetPrim());
    xformable.GetTimeSamples(&timeSamples);
    if (timeSamples.empty()) {
        return UsdTimeCode::Default();
    } else {
        return GetViewportTimeCode(viewport);
    }
}

UsdTimeCode RotationManipulator::GetViewportTimeCode(const Viewport &viewport) { return viewport.GetCurrentTimeCode(); }
