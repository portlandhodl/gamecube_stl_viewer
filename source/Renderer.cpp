#include "Renderer.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <malloc.h>

// Static member initialization
Renderer* Renderer::instance = nullptr;

// Camera constants
const f32 Camera::MIN_DISTANCE = 15.0f;
const f32 Camera::MAX_DISTANCE = 200.0f;
const f32 Camera::MAX_ROTATION_X = 1.5f;

// Camera implementation
Camera::Camera() : distance(100.0f), rotationX(0.0f), rotationY(0.0f) {
}

void Camera::SetDistance(f32 dist) {
    distance = dist;
    if (distance < MIN_DISTANCE) distance = MIN_DISTANCE;
    if (distance > MAX_DISTANCE) distance = MAX_DISTANCE;
}

void Camera::SetRotation(f32 rotX, f32 rotY) {
    rotationX = rotX;
    rotationY = rotY;

    // Constrain vertical rotation to prevent gimbal lock
    if (rotationX > MAX_ROTATION_X) rotationX = MAX_ROTATION_X;
    if (rotationX < -MAX_ROTATION_X) rotationX = -MAX_ROTATION_X;
}

void Camera::AdjustDistance(f32 delta) {
    SetDistance(distance + delta);
}

void Camera::AdjustRotation(f32 deltaX, f32 deltaY) {
    SetRotation(rotationX + deltaX, rotationY + deltaY);
}

void Camera::GetViewMatrix(Mtx view) const {
    guVector camera = {
        distance * sinf(rotationY) * cosf(rotationX),
        distance * sinf(rotationX),
        distance * cosf(rotationY) * cosf(rotationX)
    };
    guVector up = {0.0F, 1.0F, 0.0F};
    guVector look = {0.0F, 0.0F, 0.0F};

    guLookAt(view, &camera, &up, &look);
}

// LightingSystem implementation
LightingSystem::LightingSystem() {
}

void LightingSystem::Initialize() {
    SetupLights();
}

void LightingSystem::SetupLights() {
    // Set up enhanced global illumination with multiple lights
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX,
                   GX_LIGHT0 | GX_LIGHT1 | GX_LIGHT2 | GX_LIGHT3, GX_DF_CLAMP, GX_AF_NONE);

    // Enhanced ambient light for global illumination
    GX_SetChanAmbColor(GX_COLOR0A0, (GXColor){80, 80, 100, 255});

    SetupKeyLight();
    SetupFillLight();
    SetupRimLight();
    SetupBounceLight();
}

void LightingSystem::SetupKeyLight() {
    // Primary key light (warm, from upper right)
    GXLightObj keyLight;
    guVector keyDir = {0.8f, 0.6f, 1.0f};
    GX_InitLightDir(&keyLight, keyDir.x, keyDir.y, keyDir.z);
    GX_InitLightColor(&keyLight, (GXColor){255, 240, 220, 255}); // Warm white
    GX_LoadLightObj(&keyLight, GX_LIGHT0);
}

void LightingSystem::SetupFillLight() {
    // Fill light (cooler, from upper left)
    GXLightObj fillLight;
    guVector fillDir = {-0.6f, 0.4f, 0.8f};
    GX_InitLightDir(&fillLight, fillDir.x, fillDir.y, fillDir.z);
    GX_InitLightColor(&fillLight, (GXColor){180, 200, 255, 255}); // Cool blue
    GX_LoadLightObj(&fillLight, GX_LIGHT1);
}

void LightingSystem::SetupRimLight() {
    // Rim light (from behind, creates edge definition)
    GXLightObj rimLight;
    guVector rimDir = {0.2f, -0.3f, -0.9f};
    GX_InitLightDir(&rimLight, rimDir.x, rimDir.y, rimDir.z);
    GX_InitLightColor(&rimLight, (GXColor){255, 255, 255, 255}); // Bright white
    GX_LoadLightObj(&rimLight, GX_LIGHT2);
}

void LightingSystem::SetupBounceLight() {
    // Bounce light (soft upward light simulating ground reflection)
    GXLightObj bounceLight;
    guVector bounceDir = {0.0f, -1.0f, 0.2f};
    GX_InitLightDir(&bounceLight, bounceDir.x, bounceDir.y, bounceDir.z);
    GX_InitLightColor(&bounceLight, (GXColor){120, 140, 160, 255}); // Soft blue-gray
    GX_LoadLightObj(&bounceLight, GX_LIGHT3);
}

// Renderer implementation
Renderer::Renderer() : videoMode(nullptr), frameBuffer(nullptr), fifoBuffer(nullptr),
                       lighting(nullptr), initialized(false), readyForCopy(GX_FALSE) {
    instance = this;
}

Renderer::~Renderer() {
    Shutdown();
    if (instance == this) {
        instance = nullptr;
    }
}

bool Renderer::Initialize(GXRModeObj* vMode) {
    if (initialized) {
        return true;
    }

    videoMode = vMode;
    if (!videoMode) {
        printf("ERROR: Invalid video mode\n");
        return false;
    }

    // Allocate FIFO buffer
    fifoBuffer = MEM_K0_TO_K1(memalign(32, FIFO_SIZE));
    if (!fifoBuffer) {
        printf("ERROR: Failed to allocate FIFO buffer\n");
        return false;
    }
    memset(fifoBuffer, 0, FIFO_SIZE);

    // Allocate frame buffer
    frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(videoMode));
    if (!frameBuffer) {
        printf("ERROR: Failed to allocate frame buffer\n");
        return false;
    }

    // Initialize graphics pipeline
    InitializeGraphicsPipeline();

    // Initialize lighting system
    lighting = new LightingSystem();
    lighting->Initialize();

    initialized = true;
    printf("Renderer initialized successfully\n");
    return true;
}

void Renderer::Shutdown() {
    if (!initialized) {
        return;
    }

    if (lighting) {
        delete lighting;
        lighting = nullptr;
    }

    if (fifoBuffer) {
        free(fifoBuffer);
        fifoBuffer = nullptr;
    }

    // Note: frameBuffer is managed by the system, don't free it manually

    initialized = false;
}

void Renderer::InitializeGraphicsPipeline() {
    // Initialize GX
    GX_Init(fifoBuffer, FIFO_SIZE);

    // Set up frame buffer for 3D rendering
    GX_SetCopyClear((GXColor){20, 20, 40, 255}, 0x00ffffff);

    GX_SetViewport(0, 0, videoMode->fbWidth, videoMode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)videoMode->xfbHeight/(f32)videoMode->efbHeight);
    GX_SetScissor(0, 0, videoMode->fbWidth, videoMode->efbHeight);
    GX_SetDispCopySrc(0, 0, videoMode->fbWidth, videoMode->efbHeight);
    GX_SetDispCopyDst(videoMode->fbWidth, videoMode->xfbHeight);
    GX_SetCopyFilter(videoMode->aa, videoMode->sample_pattern, GX_TRUE, videoMode->vfilter);
    GX_SetFieldMode(videoMode->field_rendering,
                    ((videoMode->viHeight==2*videoMode->xfbHeight)?GX_ENABLE:GX_DISABLE));

    // Enable depth testing
    EnableDepthTesting(true);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_SetCullMode(GX_CULL_NONE); // Disable culling to ensure all faces render

    GX_CopyDisp(frameBuffer, GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    SetupProjectionMatrix();

    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    VIDEO_SetPostRetraceCallback(CopyBuffersCallback);
}

void Renderer::SetupProjectionMatrix() {
    Mtx44 projection;
    guPerspective(projection, 45, 1.33F, 1.0F, 1000.0F);
    GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
}

void Renderer::BeginFrame() {
    if (!initialized) return;

    // Clear the screen
    GX_SetCopyClear((GXColor){20, 20, 40, 255}, 0x00ffffff);

    // Set up the graphics state
    GX_SetViewport(0, 0, videoMode->fbWidth, videoMode->efbHeight, 0, 1);
    EnableDepthTesting(true);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_InvVtxCache();
    GX_InvalidateTexAll();
}

void Renderer::EndFrame() {
    if (!initialized) return;

    GX_DrawDone();
    readyForCopy = GX_TRUE;
}

void Renderer::Present() {
    if (!initialized) return;

    // The actual copy is handled by the callback
    VIDEO_WaitVSync();
}

void Renderer::RenderMesh(const Mesh* mesh, const Camera& camera) {
    if (!initialized || !mesh || !mesh->IsValid()) {
        return;
    }

    // Set up camera view matrix
    Mtx view, model, modelView;
    camera.GetViewMatrix(view);

    // Calculate model transformation (scaling and centering)
    Vector3 center = mesh->GetCenter();
    f32 maxSize = mesh->GetMaxSize();
    f32 scale = 20.0f / maxSize; // Scale to fit in a 20-unit cube

    // Set up model matrix with scaling and centering
    guMtxIdentity(model);
    guMtxScaleApply(model, model, scale, scale, scale);
    guMtxTransApply(model, model, -center.x * scale, -center.y * scale, -center.z * scale);

    // Combine view and model matrices
    guMtxConcat(view, model, modelView);
    GX_LoadPosMtxImm(modelView, GX_PNMTX0);

    // Set up vertex format
    SetupVertexFormat();

    // Render triangles
    RenderTriangles(mesh->GetTriangles(), mesh->GetTriangleCount(), mesh);
}

void Renderer::SetupVertexFormat() {
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
}

void Renderer::RenderTriangles(const Triangle* triangles, int count, const Mesh* mesh) {
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, count * 3);

    for (int i = 0; i < count; i++) {
        const Triangle* tri = &triangles[i];

        // Get material color based on surface normal
        u8 r, g, b;
        GetMaterialColor(tri->normal, r, g, b);

        for (int j = 0; j < 3; j++) {
            GX_Position3f32(tri->vertices[j].x, tri->vertices[j].y, tri->vertices[j].z);
            GX_Normal3f32(tri->normal.x, tri->normal.y, tri->normal.z);
            GX_Color4u8(r, g, b, 255); // Material color - hardware lighting will be applied
        }
    }

    GX_End();
}

void Renderer::GetMaterialColor(const Vector3& normal, u8& r, u8& g, u8& b) const {
    // Use material colors for different surface orientations to add variety
    f32 normalY = normal.y;
    f32 normalVariation = fabsf(normal.x + normal.z) * 0.3f;

    // Base bitcoin orange with subtle variations based on normal direction
    if (normalY > 0.3f) {
        // Top-facing surfaces - brighter, more golden
        r = static_cast<u8>(240 + normalVariation * 15);
        g = static_cast<u8>(160 + normalVariation * 20);
        b = static_cast<u8>(20 + normalVariation * 10);
    } else if (normalY < -0.3f) {
        // Bottom-facing surfaces - darker, more reddish
        r = static_cast<u8>(180 + normalVariation * 15);
        g = static_cast<u8>(80 + normalVariation * 15);
        b = static_cast<u8>(10 + normalVariation * 5);
    } else {
        // Side-facing surfaces - standard bitcoin orange
        r = static_cast<u8>(220 + normalVariation * 15);
        g = static_cast<u8>(140 + normalVariation * 15);
        b = static_cast<u8>(15 + normalVariation * 10);
    }

    // Clamp values to valid range
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
}

void Renderer::SetFrameBuffer(void* buffer) {
    frameBuffer = buffer;
}

void Renderer::EnableDepthTesting(bool enable) {
    if (enable) {
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    } else {
        GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    }
}

void Renderer::SetClearColor(u8 r, u8 g, u8 b, u8 a) {
    GX_SetCopyClear((GXColor){r, g, b, a}, 0x00ffffff);
}

void Renderer::CopyBuffersCallback(u32 unused) {
    if (instance && instance->readyForCopy == GX_TRUE) {
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(instance->frameBuffer, GX_TRUE);
        GX_Flush();
        instance->readyForCopy = GX_FALSE;
    }
}
