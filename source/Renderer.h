#ifndef RENDERER_H
#define RENDERER_H

#include <gccore.h>
#include "Mesh.h"

/**
 * Camera class for handling 3D view transformations
 */
class Camera {
public:
    Camera();

    void SetDistance(f32 distance);
    void SetRotation(f32 rotX, f32 rotY);
    void AdjustDistance(f32 delta);
    void AdjustRotation(f32 deltaX, f32 deltaY);

    f32 GetDistance() const { return distance; }
    f32 GetRotationX() const { return rotationX; }
    f32 GetRotationY() const { return rotationY; }

    void GetViewMatrix(Mtx view) const;

private:
    f32 distance;
    f32 rotationX;
    f32 rotationY;

    static const f32 MIN_DISTANCE;
    static const f32 MAX_DISTANCE;
    static const f32 MAX_ROTATION_X;
};

/**
 * Lighting system for enhanced 3D rendering
 */
class LightingSystem {
public:
    LightingSystem();

    void Initialize();
    void SetupLights();

private:
    void SetupKeyLight();
    void SetupFillLight();
    void SetupRimLight();
    void SetupBounceLight();
};

/**
 * 3D Renderer class for GameCube graphics
 */
class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize(GXRModeObj* videoMode);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void Present();

    void RenderMesh(const Mesh* mesh, const Camera& camera);
    void SetFrameBuffer(void* frameBuffer);

    // Rendering state
    void EnableDepthTesting(bool enable);
    void SetClearColor(u8 r, u8 g, u8 b, u8 a);

private:
    GXRModeObj* videoMode;
    void* frameBuffer;
    void* fifoBuffer;
    LightingSystem* lighting;

    bool initialized;
    vu8 readyForCopy;

    static const u32 FIFO_SIZE = 256 * 1024;

    void InitializeGraphicsPipeline();
    void SetupProjectionMatrix();
    void SetupVertexFormat();
    void RenderTriangles(const Triangle* triangles, int count, const Mesh* mesh);
    void GetMaterialColor(const Vector3& normal, u8& r, u8& g, u8& b) const;

    static void CopyBuffersCallback(u32 unused);
    static Renderer* instance; // For callback
};

#endif // RENDERER_H
