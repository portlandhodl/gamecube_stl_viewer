// Bitcoin STL Renderer for GameCube - Modular Version
// Refactored into professional class-based architecture

#include "STLViewer.h"
#include <cstdlib>

int main(int argc, char **argv) {
    // Create the main application instance
    STLViewer app;

    // Initialize the application
    if (!app.Initialize()) {
        printf("ERROR: Failed to initialize STL Viewer\n");
        printf("Press any button to exit...\n");

        // Wait for input before exiting
        PAD_Init();
        while (true) {
            PAD_ScanPads();
            if (PAD_ButtonsDown(0)) {
                break;
            }
            VIDEO_WaitVSync();
        }

        return -1;
    }

    // Run the main application loop
    app.Run();

    // Clean shutdown
    app.Shutdown();

    return 0;
}
