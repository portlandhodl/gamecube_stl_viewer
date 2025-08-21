#include "STLViewer.h"
#include "FileManager.h"
#include "Renderer.h"
#include "InputHandler.h"
#include "UI.h"
#include "Mesh.h"
#include <cstdio>
#include <cstdlib>

STLViewer::STLViewer() : fileManager(nullptr), renderer(nullptr), inputHandler(nullptr),
                         ui(nullptr), currentState(STATE_MENU), currentMesh(nullptr),
                         selectedFileIndex(0), frameBuffer(nullptr), consoleBuffer(nullptr),
                         videoMode(nullptr) {
}

STLViewer::~STLViewer() {
    Shutdown();
}

bool STLViewer::Initialize() {
    printf("Initializing STL Viewer...\n");

    // Initialize video system
    VIDEO_Init();

    // Get preferred video mode
    videoMode = VIDEO_GetPreferredMode(NULL);
    if (!videoMode) {
        printf("ERROR: Failed to get video mode\n");
        return false;
    }

    // Allocate frame buffer for 3D rendering
    frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(videoMode));
    if (!frameBuffer) {
        printf("ERROR: Failed to allocate frame buffer\n");
        return false;
    }

    // Allocate console buffer for menu display
    consoleBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(videoMode));
    if (!consoleBuffer) {
        printf("ERROR: Failed to allocate console buffer\n");
        return false;
    }

    // Set up video
    VIDEO_Configure(videoMode);
    VIDEO_SetNextFramebuffer(consoleBuffer); // Start with console buffer
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(videoMode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    // Initialize components
    fileManager = new FileManager();
    if (!fileManager->Initialize()) {
        printf("WARNING: File system initialization failed\n");
        // Continue anyway - we might still have local files
    }

    renderer = new Renderer();
    if (!renderer->Initialize(videoMode)) {
        printf("ERROR: Renderer initialization failed\n");
        return false;
    }
    renderer->SetFrameBuffer(frameBuffer);

    inputHandler = new InputHandler();
    inputHandler->Initialize();

    ui = new UI();
    if (!ui->Initialize(videoMode, consoleBuffer)) {
        printf("ERROR: UI initialization failed\n");
        return false;
    }

    // Initialize mesh
    currentMesh = new Mesh();

    // Set initial state
    currentState = STATE_MENU;
    selectedFileIndex = 0;

    printf("STL Viewer initialized successfully!\n");
    return true;
}

void STLViewer::Run() {
    if (!videoMode || !inputHandler || !ui) {
        printf("ERROR: STL Viewer not properly initialized\n");
        return;
    }

    printf("Starting STL Viewer main loop...\n");

    // Show initial menu
    SwitchToMenuMode();

    // Main application loop
    while (true) {
        inputHandler->Update();

        // Check for exit
        if (inputHandler->IsExitRequested()) {
            printf("Exit requested by user\n");
            break;
        }

        // Update based on current state
        switch (currentState) {
            case STATE_MENU:
                UpdateMenu();
                break;

            case STATE_RENDERING:
                UpdateRendering();
                break;
        }

        // Present frame
        if (currentState == STATE_RENDERING) {
            renderer->Present();
        } else {
            VIDEO_WaitVSync();
        }
    }

    printf("Exiting STL Viewer...\n");
}

void STLViewer::Shutdown() {
    if (currentMesh) {
        delete currentMesh;
        currentMesh = nullptr;
    }

    if (ui) {
        delete ui;
        ui = nullptr;
    }

    if (inputHandler) {
        delete inputHandler;
        inputHandler = nullptr;
    }

    if (renderer) {
        delete renderer;
        renderer = nullptr;
    }

    if (fileManager) {
        delete fileManager;
        fileManager = nullptr;
    }

    // Note: frameBuffer and consoleBuffer are managed by the system
}

void STLViewer::UpdateMenu() {
    const InputState& input = inputHandler->GetCurrentState();
    bool needsRedraw = false;

    // Handle file navigation
    if (input.upPressed && fileManager->GetFileCount() > 0) {
        selectedFileIndex = (selectedFileIndex - 1 + fileManager->GetFileCount()) % fileManager->GetFileCount();
        needsRedraw = true;
    }

    if (input.downPressed && fileManager->GetFileCount() > 0) {
        selectedFileIndex = (selectedFileIndex + 1) % fileManager->GetFileCount();
        needsRedraw = true;
    }

    // Handle file selection
    if (input.aPressed && fileManager->GetFileCount() > 0) {
        const FileEntry* selectedFile = fileManager->GetFile(selectedFileIndex);
        if (selectedFile) {
            ui->ShowLoadingScreen(selectedFile->name);

            if (currentMesh->LoadFromSTL(selectedFile->path.c_str())) {
                printf("Successfully loaded: %s\n", selectedFile->name.c_str());
                SwitchToRenderMode();
                return;
            } else {
                printf("Failed to load: %s\n", selectedFile->name.c_str());
                ui->ShowStatusBox("Failed to load STL file!");
                needsRedraw = true;
            }
        }
    }

    // Redraw menu if needed
    if (needsRedraw) {
        ui->ShowMainMenu(*fileManager, selectedFileIndex);
    }
}

void STLViewer::UpdateRendering() {
    const InputState& input = inputHandler->GetCurrentState();
    static Camera camera; // Static to maintain state between frames

    // Handle return to menu
    if (input.bPressed) {
        SwitchToMenuMode();
        return;
    }

    // Handle camera controls
    if (inputHandler->HasCameraRotationInput()) {
        f32 deltaX, deltaY;
        inputHandler->GetCameraRotationDelta(deltaX, deltaY);
        camera.AdjustRotation(deltaX, deltaY);
    }

    // Handle zoom
    f32 zoomDelta = inputHandler->GetZoomDelta();
    if (zoomDelta != 0.0f) {
        camera.AdjustDistance(zoomDelta);
    }

    // Render the scene
    renderer->BeginFrame();
    renderer->RenderMesh(currentMesh, camera);
    renderer->EndFrame();
}

void STLViewer::SwitchToMenuMode() {
    currentState = STATE_MENU;
    VIDEO_SetNextFramebuffer(consoleBuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();

    // Show the menu
    ui->ShowMainMenu(*fileManager, selectedFileIndex);
}

void STLViewer::SwitchToRenderMode() {
    currentState = STATE_RENDERING;
    VIDEO_SetNextFramebuffer(frameBuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
}
