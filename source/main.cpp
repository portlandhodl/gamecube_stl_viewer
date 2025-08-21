// Bitcoin STL Renderer for GameCube
// Using stl_reader library for robust STL file handling

#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <cmath>
#include <cstdio>
#include <gccore.h>
#include <fat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>


#define MAX_FILES 100
#define FIFO_SIZE (256*1024)

typedef struct {
    f32 x, y, z;
} Vector3;

typedef struct {
    Vector3 normal;
    Vector3 vertices[3];
} Triangle;

typedef struct {
    Triangle* triangles;
    int triangleCount;
} Mesh;

typedef struct {
    char name[256];
    char path[512];
} FileEntry;

// Global variables
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
static void *frameBuffer;
static vu8 readyForCopy;
static Mesh currentMesh;
static FileEntry files[MAX_FILES];
static int fileCount = 0;
static int selectedFile = 0;
static int currentState = 0; // 0 = menu, 1 = rendering
static f32 cameraDistance = 100.0f;
static f32 cameraRotX = 0.0f;
static f32 cameraRotY = 0.0f;

// Function prototypes
int load_stl_file(const char* filename, Mesh* mesh);
void draw_mesh();
void init_graphics();
static void copy_buffers(u32 unused);
void scan_files();

int load_stl_file(const char* filename, Mesh* mesh) {
    printf("Loading STL file: %s\n", filename);

    // Clean up existing mesh
    if (mesh->triangles) {
        free(mesh->triangles);
        mesh->triangles = NULL;
        mesh->triangleCount = 0;
    }

    // First, let's check basic file properties
    FILE* testFile = fopen(filename, "rb");
    if (!testFile) {
        printf("ERROR: Cannot open file: %s\n", filename);
        return 0;
    }

    // Get file size
    fseek(testFile, 0, SEEK_END);
    long fileSize = ftell(testFile);
    fseek(testFile, 0, SEEK_SET);
    printf("File size: %ld bytes\n", fileSize);

    // Read first 80 bytes to check format
    char header[81] = {0};
    if (fread(header, 1, 80, testFile) == 80) {
        printf("STL Header: %.80s\n", header);

        // Check if it looks like ASCII (contains "solid" keyword)
        bool isASCII = (strstr(header, "solid") != NULL);
        printf("Detected format: %s\n", isASCII ? "ASCII" : "BINARY");

        if (!isASCII) {
            // For binary, read triangle count
            unsigned char countBytes[4];
            if (fread(countBytes, 1, 4, testFile) == 4) {
                unsigned int triangleCount = countBytes[0] | (countBytes[1] << 8) | (countBytes[2] << 16) | (countBytes[3] << 24);
                printf("Binary STL triangle count: %u\n", triangleCount);

                // Validate expected file size
                long expectedSize = 80 + 4 + (triangleCount * 50);  // header + count + (12*4 + 2) per triangle
                printf("Expected size: %ld bytes, Actual size: %ld bytes\n", expectedSize, fileSize);

                if (fileSize < expectedSize) {
                    printf("WARNING: File appears truncated!\n");
                }
            }
        }
    }
    fclose(testFile);

    // Using manual binary STL parser (reliable with our bitcoin.stl file)
    printf("Using manual binary STL parser...\n");

    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("ERROR: Failed to reopen file for manual parsing\n");
        return 0;
    }

    // Skip STL header (80 bytes)
    fseek(file, 80, SEEK_SET);

    // Read triangle count (4 bytes, little-endian)
    unsigned char countBytes[4];
    if (fread(countBytes, 1, 4, file) != 4) {
        printf("ERROR: Failed to read triangle count\n");
        fclose(file);
        return 0;
    }

    // Convert from little-endian to host byte order
    unsigned int triangleCount = countBytes[0] | (countBytes[1] << 8) | (countBytes[2] << 16) | (countBytes[3] << 24);
    printf("Manual parser: Reading %u triangles\n", triangleCount);

    // Basic sanity check
    if (triangleCount > 1000000) {
        printf("WARNING: Triangle count %u seems excessive, limiting to 100000\n", triangleCount);
        triangleCount = 100000;
    }

    if (triangleCount == 0) {
        printf("WARNING: STL file contains no triangles\n");
        fclose(file);
        return 0;
    }

    // Allocate memory for our Triangle structure
    mesh->triangles = (Triangle*)malloc(triangleCount * sizeof(Triangle));
    if (!mesh->triangles) {
        printf("ERROR: Failed to allocate memory for triangles\n");
        fclose(file);
        return 0;
    }

    mesh->triangleCount = triangleCount;

    // Track bounding box for scaling
    f32 minX = 1e9f, maxX = -1e9f;
    f32 minY = 1e9f, maxY = -1e9f;
    f32 minZ = 1e9f, maxZ = -1e9f;

    // Read triangles using manual parsing with proper endianness
    for (unsigned int i = 0; i < triangleCount; i++) {
        Triangle *tri = &mesh->triangles[i];

        // Read normal vector (little-endian floats)
        union { float f; unsigned int i; } converter;
        unsigned char bytes[4];

        for (int n = 0; n < 3; n++) {
            if (fread(bytes, 1, 4, file) != 4) {
                printf("ERROR: Failed to read normal component %d for triangle %u\n", n, i);
                free(mesh->triangles);
                mesh->triangles = NULL;
                fclose(file);
                return 0;
            }
            converter.i = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            if (n == 0) tri->normal.x = converter.f;
            else if (n == 1) tri->normal.y = converter.f;
            else tri->normal.z = converter.f;
        }

        // Read vertices (little-endian floats)
        for (int j = 0; j < 3; j++) {
            for (int v = 0; v < 3; v++) {
                if (fread(bytes, 1, 4, file) != 4) {
                    printf("ERROR: Failed to read vertex %d component %d for triangle %u\n", j, v, i);
                    free(mesh->triangles);
                    mesh->triangles = NULL;
                    fclose(file);
                    return 0;
                }
                converter.i = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                if (v == 0) tri->vertices[j].x = converter.f;
                else if (v == 1) tri->vertices[j].y = converter.f;
                else tri->vertices[j].z = converter.f;
            }

            // Update bounding box
            if (tri->vertices[j].x < minX) minX = tri->vertices[j].x;
            if (tri->vertices[j].x > maxX) maxX = tri->vertices[j].x;
            if (tri->vertices[j].y < minY) minY = tri->vertices[j].y;
            if (tri->vertices[j].y > maxY) maxY = tri->vertices[j].y;
            if (tri->vertices[j].z < minZ) minZ = tri->vertices[j].z;
            if (tri->vertices[j].z > maxZ) maxZ = tri->vertices[j].z;
        }

        // Skip attribute byte count
        fseek(file, 2, SEEK_CUR);
    }

    fclose(file);

    // Print model bounds for debugging
    printf("Model bounds: X(%.2f to %.2f) Y(%.2f to %.2f) Z(%.2f to %.2f)\n",
           minX, maxX, minY, maxY, minZ, maxZ);

    f32 sizeX = maxX - minX;
    f32 sizeY = maxY - minY;
    f32 sizeZ = maxZ - minZ;
    f32 maxSize = (sizeX > sizeY) ? ((sizeX > sizeZ) ? sizeX : sizeZ) : ((sizeY > sizeZ) ? sizeY : sizeZ);
    printf("Model size: %.2f x %.2f x %.2f (max: %.2f)\n", sizeX, sizeY, sizeZ, maxSize);

    printf("STL loaded successfully using manual parser!\n");
    return 1;
}

void scan_files() {
    DIR *dir;
    struct dirent *entry;

    fileCount = 0;

    // Try to open root directory on SD card first
    dir = opendir("sd:/");
    if (dir == NULL) {
        // Try USB if SD fails
        dir = opendir("usb:/");
    }

    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL && fileCount < MAX_FILES) {
            char *ext = strrchr(entry->d_name, '.');
            if (ext && strcasecmp(ext, ".stl") == 0) {
                strcpy(files[fileCount].name, entry->d_name);
                snprintf(files[fileCount].path, sizeof(files[fileCount].path),
                         dir == opendir("sd:/") ? "sd:/%s" : "usb:/%s", entry->d_name);
                fileCount++;
            }
        }
        closedir(dir);
    }

    // Also check current directory for bitcoin.stl
    if (access("bitcoin.stl", F_OK) == 0) {
        strcpy(files[fileCount].name, "bitcoin.stl");
        strcpy(files[fileCount].path, "bitcoin.stl");
        fileCount++;
    }
}

void init_graphics() {
    void *fifoBuffer = NULL;
    Mtx44 projection;

    fifoBuffer = MEM_K0_TO_K1(memalign(32, FIFO_SIZE));
    memset(fifoBuffer, 0, FIFO_SIZE);

    GX_Init(fifoBuffer, FIFO_SIZE);

    // Set up frame buffer for 3D rendering
    frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    GX_SetCopyClear((GXColor){20, 20, 40, 255}, 0x00ffffff);

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetDispCopyYScale((f32)rmode->xfbHeight/(f32)rmode->efbHeight);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, rmode->xfbHeight);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering,
                    ((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

    // Enable depth testing with less aggressive culling for solid rendering
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_SetCullMode(GX_CULL_NONE); // Disable culling to ensure all faces render

    GX_CopyDisp(frameBuffer, GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    // Set up projection matrix with wider far plane
    guPerspective(projection, 45, 1.33F, 1.0F, 1000.0F);
    GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);

    // Set up enhanced global illumination with multiple lights
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX,
                   GX_LIGHT0 | GX_LIGHT1 | GX_LIGHT2 | GX_LIGHT3, GX_DF_CLAMP, GX_AF_NONE);

    // Enhanced ambient light for global illumination
    GX_SetChanAmbColor(GX_COLOR0A0, (GXColor){80, 80, 100, 255});

    // Primary key light (warm, from upper right)
    GXLightObj keyLight;
    guVector keyDir = {0.8f, 0.6f, 1.0f};
    GX_InitLightDir(&keyLight, keyDir.x, keyDir.y, keyDir.z);
    GX_InitLightColor(&keyLight, (GXColor){255, 240, 220, 255}); // Warm white
    GX_LoadLightObj(&keyLight, GX_LIGHT0);

    // Fill light (cooler, from upper left)
    GXLightObj fillLight;
    guVector fillDir = {-0.6f, 0.4f, 0.8f};
    GX_InitLightDir(&fillLight, fillDir.x, fillDir.y, fillDir.z);
    GX_InitLightColor(&fillLight, (GXColor){180, 200, 255, 255}); // Cool blue
    GX_LoadLightObj(&fillLight, GX_LIGHT1);

    // Rim light (from behind, creates edge definition)
    GXLightObj rimLight;
    guVector rimDir = {0.2f, -0.3f, -0.9f};
    GX_InitLightDir(&rimLight, rimDir.x, rimDir.y, rimDir.z);
    GX_InitLightColor(&rimLight, (GXColor){255, 255, 255, 255}); // Bright white
    GX_LoadLightObj(&rimLight, GX_LIGHT2);

    // Bounce light (soft upward light simulating ground reflection)
    GXLightObj bounceLight;
    guVector bounceDir = {0.0f, -1.0f, 0.2f};
    GX_InitLightDir(&bounceLight, bounceDir.x, bounceDir.y, bounceDir.z);
    GX_InitLightColor(&bounceLight, (GXColor){120, 140, 160, 255}); // Soft blue-gray
    GX_LoadLightObj(&bounceLight, GX_LIGHT3);

    GX_SetNumTexGens(0);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    VIDEO_SetPostRetraceCallback(copy_buffers);
}

void draw_mesh() {
    if (currentMesh.triangles == NULL || currentMesh.triangleCount == 0) {
        return;
    }

    // Clear the screen
    GX_SetCopyClear((GXColor){20, 20, 40, 255}, 0x00ffffff);

    // Set up camera
    Mtx view, model, modelView;
    guVector camera = {
        cameraDistance * sinf(cameraRotY) * cosf(cameraRotX),
        cameraDistance * sinf(cameraRotX),
        cameraDistance * cosf(cameraRotY) * cosf(cameraRotX)
    };
    guVector up = {0.0F, 1.0F, 0.0F};
    guVector look = {0.0F, 0.0F, 0.0F};

    guLookAt(view, &camera, &up, &look);

    // Calculate model bounds and scaling
    f32 minX = 1e9f, maxX = -1e9f;
    f32 minY = 1e9f, maxY = -1e9f;
    f32 minZ = 1e9f, maxZ = -1e9f;

    for (int i = 0; i < currentMesh.triangleCount; i++) {
        for (int j = 0; j < 3; j++) {
            if (currentMesh.triangles[i].vertices[j].x < minX) minX = currentMesh.triangles[i].vertices[j].x;
            if (currentMesh.triangles[i].vertices[j].x > maxX) maxX = currentMesh.triangles[i].vertices[j].x;
            if (currentMesh.triangles[i].vertices[j].y < minY) minY = currentMesh.triangles[i].vertices[j].y;
            if (currentMesh.triangles[i].vertices[j].y > maxY) maxY = currentMesh.triangles[i].vertices[j].y;
            if (currentMesh.triangles[i].vertices[j].z < minZ) minZ = currentMesh.triangles[i].vertices[j].z;
            if (currentMesh.triangles[i].vertices[j].z > maxZ) maxZ = currentMesh.triangles[i].vertices[j].z;
        }
    }

    f32 sizeX = maxX - minX;
    f32 sizeY = maxY - minY;
    f32 sizeZ = maxZ - minZ;
    f32 maxSize = (sizeX > sizeY) ? ((sizeX > sizeZ) ? sizeX : sizeZ) : ((sizeY > sizeZ) ? sizeY : sizeZ);
    f32 scale = 20.0f / maxSize; // Scale to fit in a 20-unit cube

    f32 centerX = (minX + maxX) * 0.5f;
    f32 centerY = (minY + maxY) * 0.5f;
    f32 centerZ = (minZ + maxZ) * 0.5f;

    // Set up model matrix with scaling and centering
    guMtxIdentity(model);
    guMtxScaleApply(model, model, scale, scale, scale);
    guMtxTransApply(model, model, -centerX * scale, -centerY * scale, -centerZ * scale);

    // Combine view and model matrices
    guMtxConcat(view, model, modelView);
    GX_LoadPosMtxImm(modelView, GX_PNMTX0);

    // Set up the graphics state properly
    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);
    GX_InvVtxCache();
    GX_InvalidateTexAll();

    // Set up vertex format
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    // Draw triangles with enhanced material properties
    GX_Begin(GX_TRIANGLES, GX_VTXFMT0, currentMesh.triangleCount * 3);

    for (int i = 0; i < currentMesh.triangleCount; i++) {
        Triangle *tri = &currentMesh.triangles[i];

        // Use material colors for different surface orientations to add variety
        f32 normalY = tri->normal.y;
        f32 normalVariation = fabsf(tri->normal.x + tri->normal.z) * 0.3f;

        // Base bitcoin orange with subtle variations based on normal direction
        u8 r, g, b;
        if (normalY > 0.3f) {
            // Top-facing surfaces - brighter, more golden
            r = (u8)(240 + normalVariation * 15);
            g = (u8)(160 + normalVariation * 20);
            b = (u8)(20 + normalVariation * 10);
        } else if (normalY < -0.3f) {
            // Bottom-facing surfaces - darker, more reddish
            r = (u8)(180 + normalVariation * 15);
            g = (u8)(80 + normalVariation * 15);
            b = (u8)(10 + normalVariation * 5);
        } else {
            // Side-facing surfaces - standard bitcoin orange
            r = (u8)(220 + normalVariation * 15);
            g = (u8)(140 + normalVariation * 15);
            b = (u8)(15 + normalVariation * 10);
        }

        // Clamp values to valid range
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;

        for (int j = 0; j < 3; j++) {
            GX_Position3f32(tri->vertices[j].x, tri->vertices[j].y, tri->vertices[j].z);
            GX_Normal3f32(tri->normal.x, tri->normal.y, tri->normal.z);
            GX_Color4u8(r, g, b, 255); // Material color - hardware lighting will be applied
        }
    }

    GX_End();
    GX_DrawDone();
    readyForCopy = GX_TRUE;
}

static void copy_buffers(u32 count __attribute__ ((unused))) {
    if (readyForCopy == GX_TRUE) {
        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(frameBuffer, GX_TRUE);
        GX_Flush();
        readyForCopy = GX_FALSE;
    }
}

int main(int argc, char **argv) {
    // Initialize video system
    VIDEO_Init();
    PAD_Init();

    // Get preferred video mode
    rmode = VIDEO_GetPreferredMode(NULL);

    // Allocate memory for display
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    // Initialize console for text output
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth*VI_DISPLAY_PIX_SZ);

    // Set up video
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    printf("\x1b[2;0H");
    printf("Bitcoin STL Renderer for GameCube\n");
    printf("==================================\n\n");

    // Initialize filesystem
    if(fatInitDefault()) {
        printf("Filesystem initialized successfully\n");
        scan_files();

        if (fileCount > 0) {
            printf("Found %d STL file(s):\n", fileCount);
            for (int i = 0; i < fileCount; i++) {
                printf("  %d. %s\n", i + 1, files[i].name);
            }
            printf("\nUse UP/DOWN to select, A to load, B to return\n");
            printf("In 3D view: Analog stick for 360° rotation, C-stick for fine control\n");
            printf("L/R triggers to zoom, Z button for fast zoom, START to exit\n");
        } else {
            printf("No STL files found.\n");
            printf("Place .stl files in SD card root or current directory.\n");
        }
    } else {
        printf("Filesystem initialization failed\n");
        printf("Insert SD card with STL files\n");
    }

    // Initialize mesh
    currentMesh.triangles = NULL;
    currentMesh.triangleCount = 0;

    // Initialize 3D graphics
    init_graphics();

    while(1) {
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsDown(0);
        u32 held = PAD_ButtonsHeld(0);

        if (pressed & PAD_BUTTON_START) {
            exit(0);
        }

        if (currentState == 0) { // Menu state
            if (pressed & PAD_BUTTON_UP && fileCount > 0) {
                selectedFile = (selectedFile - 1 + fileCount) % fileCount;
                // Clear the selection area and redraw
                printf("\x1b[%d;0H                                                          ", 8 + fileCount + 2);
                printf("\x1b[%d;0HSelected: %s", 8 + fileCount + 2, files[selectedFile].name);
            }
            if (pressed & PAD_BUTTON_DOWN && fileCount > 0) {
                selectedFile = (selectedFile + 1) % fileCount;
                // Clear the selection area and redraw
                printf("\x1b[%d;0H                                                          ", 8 + fileCount + 2);
                printf("\x1b[%d;0HSelected: %s", 8 + fileCount + 2, files[selectedFile].name);
            }
            if (pressed & PAD_BUTTON_A && fileCount > 0) {
                // Clear loading area
                printf("\x1b[%d;0H                                                          ", 8 + fileCount + 3);
                printf("\x1b[%d;0H                                                          ", 8 + fileCount + 4);
                printf("\x1b[%d;0HLoading %s...", 8 + fileCount + 3, files[selectedFile].name);
                if (load_stl_file(files[selectedFile].path, &currentMesh)) {
                    currentState = 1; // Switch to 3D view
                    VIDEO_SetNextFramebuffer(frameBuffer);
                    VIDEO_SetBlack(FALSE);
                    VIDEO_Flush();
                }
            }
        } else { // 3D rendering state
            if (pressed & PAD_BUTTON_B) {
                currentState = 0; // Back to menu
                VIDEO_SetNextFramebuffer(xfb);
                VIDEO_SetBlack(FALSE);
                VIDEO_Flush();
            }

            // Full 360-degree analog joystick camera controls
            s8 stickX = PAD_StickX(0);
            s8 stickY = PAD_StickY(0);
            s8 cStickX = PAD_SubStickX(0);
            s8 cStickY = PAD_SubStickY(0);

            // Main analog stick for rotation (full 360-degree range)
            if (abs(stickX) > 10) { // Deadzone
                cameraRotY += (f32)stickX * 0.002f; // Smooth rotation
            }
            if (abs(stickY) > 10) { // Deadzone
                cameraRotX += (f32)stickY * 0.001f; // Smooth vertical rotation
            }

            // C-stick for additional rotation control
            if (abs(cStickX) > 10) { // Deadzone
                cameraRotY += (f32)cStickX * 0.001f;
            }
            if (abs(cStickY) > 10) { // Deadzone
                cameraRotX += (f32)cStickY * 0.0005f;
            }

            // D-pad for fine adjustment and zoom
            if (held & PAD_BUTTON_LEFT) cameraRotY -= 0.02f;
            if (held & PAD_BUTTON_RIGHT) cameraRotY += 0.02f;
            if (held & PAD_BUTTON_UP) cameraRotX -= 0.015f;
            if (held & PAD_BUTTON_DOWN) cameraRotX += 0.015f;

            // Triggers for zoom control
            if (held & PAD_TRIGGER_L) {
                cameraDistance -= 2.0f;
                if (cameraDistance < 20.0f) cameraDistance = 20.0f;
            }
            if (held & PAD_TRIGGER_R) {
                cameraDistance += 2.0f;
                if (cameraDistance > 200.0f) cameraDistance = 200.0f;
            }

            // Shoulder buttons for quick zoom
            if (held & PAD_TRIGGER_Z) {
                cameraDistance -= 4.0f;
                if (cameraDistance < 15.0f) cameraDistance = 15.0f;
            }

            // Constrain vertical rotation to prevent gimbal lock
            if (cameraRotX > 1.5f) cameraRotX = 1.5f;
            if (cameraRotX < -1.5f) cameraRotX = -1.5f;

            draw_mesh();
        }

        VIDEO_WaitVSync();
    }

    return 0;
}
