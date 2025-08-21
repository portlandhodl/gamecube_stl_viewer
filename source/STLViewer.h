#ifndef STL_VIEWER_H
#define STL_VIEWER_H

#include <gccore.h>
#include <vector>
#include <string>

// Forward declarations
class Mesh;
class FileManager;
class Renderer;
class InputHandler;
class UI;

/**
 * Main application class that coordinates all components
 */
class STLViewer {
public:
    STLViewer();
    ~STLViewer();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    enum AppState {
        STATE_MENU = 0,
        STATE_RENDERING = 1
    };

    // Core components
    FileManager* fileManager;
    Renderer* renderer;
    InputHandler* inputHandler;
    UI* ui;

    // Application state
    AppState currentState;
    Mesh* currentMesh;
    int selectedFileIndex;

    // Video system
    void* frameBuffer;
    void* consoleBuffer;
    GXRModeObj* videoMode;

    void UpdateMenu();
    void UpdateRendering();
    void SwitchToMenuMode();
    void SwitchToRenderMode();
};

#endif // STL_VIEWER_H
