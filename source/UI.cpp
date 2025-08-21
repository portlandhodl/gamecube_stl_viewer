#include "UI.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iomanip>

UI::UI() : videoMode(nullptr), consoleBuffer(nullptr), initialized(false),
           consoleWidth(80), consoleHeight(24) {
}

UI::~UI() {
    Shutdown();
}

bool UI::Initialize(GXRModeObj* vMode, void* buffer) {
    videoMode = vMode;
    consoleBuffer = buffer;

    if (!videoMode || !consoleBuffer) {
        return false;
    }

    InitializeConsole();
    initialized = true;
    return true;
}

void UI::Shutdown() {
    initialized = false;
}

void UI::ShowMainMenu(const FileManager& fileManager, int selectedIndex) {
    ClearScreen();

    // Title
    PrintCentered(2, "Bitcoin STL Renderer for GameCube");
    PrintCentered(3, "==================================");

    // File selection box
    ShowFileSelectionBox(fileManager, selectedIndex);

    // Instructions
    int instructionY = 18;
    PrintCentered(instructionY++, "Controls:");
    PrintCentered(instructionY++, "UP/DOWN - Navigate files  |  A - Load file  |  START - Exit");
    PrintCentered(instructionY++, "3D View: Analog stick - Rotate  |  L/R - Zoom  |  B - Back to menu");
}

void UI::ShowFileSelectionBox(const FileManager& fileManager, int selectedIndex) {
    const int boxX = 10;
    const int boxY = 6;
    const int boxWidth = 60;
    const int boxHeight = 10;

    // Draw main file selection box
    UIBox fileBox(boxX, boxY, boxWidth, boxHeight, "STL Files");
    DrawBox(fileBox);

    const auto& files = fileManager.GetFiles();

    if (files.empty()) {
        PrintAt(boxX + 2, boxY + 3, "No STL files found.");
        PrintAt(boxX + 2, boxY + 4, "Place .stl files on SD card or USB drive.");
    } else {
        // Show file list
        DrawFileList(files, selectedIndex, boxX + 2, boxY + 2, boxHeight - 3);

        // Show selected file info box
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(files.size())) {
            const FileEntry& selectedFile = files[selectedIndex];

            UIBox infoBox(boxX, boxY + boxHeight + 1, boxWidth, 4, "Selected File");
            DrawBox(infoBox);

            std::string filename = "File: " + selectedFile.name;
            std::string filesize = "Size: " + FormatFileSize(selectedFile.size);

            PrintAt(boxX + 2, boxY + boxHeight + 3, TruncateText(filename, boxWidth - 4));
            PrintAt(boxX + 2, boxY + boxHeight + 4, filesize);
        }
    }
}

void UI::ShowStatusBox(const std::string& status) {
    const int boxX = 15;
    const int boxY = 10;
    const int boxWidth = 50;
    const int boxHeight = 4;

    UIBox statusBox(boxX, boxY, boxWidth, boxHeight, "Status");
    DrawBox(statusBox);

    PrintAt(boxX + 2, boxY + 2, TruncateText(status, boxWidth - 4));
}

void UI::ShowLoadingScreen(const std::string& filename) {
    ClearScreen();

    PrintCentered(10, "Loading STL File...");
    PrintCentered(12, filename);
    PrintCentered(14, "Please wait...");

    // Simple loading animation
    static int loadingFrame = 0;
    const char* loadingChars = "|/-\\";
    char loadingStr[3] = {loadingChars[loadingFrame % 4], ' ', '\0'};
    PrintCentered(16, loadingStr);
    loadingFrame++;
}

void UI::ClearScreen() {
    if (!initialized) return;

    printf("\x1b[2J"); // Clear entire screen
    printf("\x1b[H");  // Move cursor to home position
}

void UI::RefreshDisplay() {
    // Force console refresh if needed
}

void UI::SetConsolePosition(int x, int y) {
    printf("\x1b[%d;%dH", y + 1, x + 1); // ANSI escape sequence (1-based)
}

void UI::DrawBox(const UIBox& box) {
    // Draw top border
    SetConsolePosition(box.x, box.y);
    printf("%c", BORDER_CORNER);
    DrawHorizontalLine(box.x + 1, box.y, box.width - 2);
    printf("%c", BORDER_CORNER);

    // Draw title if present
    if (!box.title.empty()) {
        int titleX = box.x + (box.width - static_cast<int>(box.title.length())) / 2;
        if (titleX > box.x + 1) {
            SetConsolePosition(titleX - 1, box.y);
            printf(" %s ", box.title.c_str());
        }
    }

    // Draw sides
    for (int i = 1; i < box.height - 1; i++) {
        SetConsolePosition(box.x, box.y + i);
        printf("%c", BORDER_VERTICAL);
        SetConsolePosition(box.x + box.width - 1, box.y + i);
        printf("%c", BORDER_VERTICAL);
    }

    // Draw bottom border
    SetConsolePosition(box.x, box.y + box.height - 1);
    printf("%c", BORDER_CORNER);
    DrawHorizontalLine(box.x + 1, box.y + box.height - 1, box.width - 2);
    printf("%c", BORDER_CORNER);
}

void UI::DrawBorder(int x, int y, int width, int height) {
    UIBox box(x, y, width, height);
    DrawBox(box);
}

void UI::DrawTitle(const std::string& title, int x, int y, int width) {
    int titleX = x + (width - static_cast<int>(title.length())) / 2;
    PrintAt(titleX, y, title);
}

void UI::DrawFileList(const std::vector<FileEntry>& files, int selectedIndex, int x, int y, int maxItems) {
    int startIndex = 0;
    int endIndex = static_cast<int>(files.size());

    // Calculate scroll window if needed
    if (endIndex > maxItems) {
        startIndex = selectedIndex - maxItems / 2;
        if (startIndex < 0) startIndex = 0;
        if (startIndex + maxItems > endIndex) startIndex = endIndex - maxItems;
        endIndex = startIndex + maxItems;
    }

    for (int i = startIndex; i < endIndex && i < static_cast<int>(files.size()); i++) {
        int displayY = y + (i - startIndex);

        std::string displayText;
        if (i == selectedIndex) {
            displayText = std::string(1, SELECTION_MARKER) + " " + files[i].name;
        } else {
            displayText = "  " + files[i].name;
        }

        PrintAt(x, displayY, TruncateText(displayText, 56));
    }

    // Show scroll indicators if needed
    if (files.size() > static_cast<size_t>(maxItems)) {
        if (startIndex > 0) {
            PrintAt(x + 54, y - 1, "^");
        }
        if (endIndex < static_cast<int>(files.size())) {
            PrintAt(x + 54, y + maxItems, "v");
        }
    }
}

void UI::PrintAt(int x, int y, const std::string& text) {
    SetConsolePosition(x, y);
    printf("%s", text.c_str());
}

void UI::PrintCentered(int y, const std::string& text, int width) {
    int x = (width - static_cast<int>(text.length())) / 2;
    if (x < 0) x = 0;
    PrintAt(x, y, text);
}

std::string UI::TruncateText(const std::string& text, int maxLength) const {
    if (static_cast<int>(text.length()) <= maxLength) {
        return text;
    }

    if (maxLength <= 3) {
        return text.substr(0, maxLength);
    }

    return text.substr(0, maxLength - 3) + "...";
}

std::string UI::FormatFileSize(long bytes) const {
    std::ostringstream oss;

    if (bytes < 1024) {
        oss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
    } else {
        oss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
    }

    return oss.str();
}

void UI::InitializeConsole() {
    if (!videoMode || !consoleBuffer) return;

    console_init(consoleBuffer, 20, 20, videoMode->fbWidth, videoMode->xfbHeight,
                 videoMode->fbWidth * VI_DISPLAY_PIX_SZ);

    // Calculate console dimensions based on video mode
    consoleWidth = (videoMode->fbWidth - 40) / 8; // Approximate character width
    consoleHeight = (videoMode->xfbHeight - 40) / 16; // Approximate character height

    if (consoleWidth > 80) consoleWidth = 80;
    if (consoleHeight > 30) consoleHeight = 30;
}

void UI::DrawHorizontalLine(int x, int y, int length, char character) {
    SetConsolePosition(x, y);
    for (int i = 0; i < length; i++) {
        printf("%c", character);
    }
}

void UI::DrawVerticalLine(int x, int y, int length, char character) {
    for (int i = 0; i < length; i++) {
        SetConsolePosition(x, y + i);
        printf("%c", character);
    }
}
