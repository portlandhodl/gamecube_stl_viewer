#ifndef UI_H
#define UI_H

#include <gccore.h>
#include <string>
#include <vector>
#include "FileManager.h"

/**
 * Menu item structure for styled menu display
 */
struct MenuItem {
    std::string text;
    bool isSelected;
    bool isEnabled;

    MenuItem(const std::string& t, bool enabled = true)
        : text(t), isSelected(false), isEnabled(enabled) {}
};

/**
 * UI Box structure for creating styled interface elements
 */
struct UIBox {
    int x, y, width, height;
    std::string title;
    std::vector<std::string> content;
    bool hasBorder;

    UIBox(int x_, int y_, int w, int h, const std::string& t = "")
        : x(x_), y(y_), width(w), height(h), title(t), hasBorder(true) {}
};

/**
 * User Interface class for menu and status display
 */
class UI {
public:
    UI();
    ~UI();

    bool Initialize(GXRModeObj* videoMode, void* consoleBuffer);
    void Shutdown();

    // Menu display
    void ShowMainMenu(const FileManager& fileManager, int selectedIndex);
    void ShowFileSelectionBox(const FileManager& fileManager, int selectedIndex);
    void ShowStatusBox(const std::string& status);
    void ShowLoadingScreen(const std::string& filename);

    // Screen management
    void ClearScreen();
    void RefreshDisplay();
    void SetConsolePosition(int x, int y);

    // Styled drawing functions
    void DrawBox(const UIBox& box);
    void DrawBorder(int x, int y, int width, int height);
    void DrawTitle(const std::string& title, int x, int y, int width);
    void DrawFileList(const std::vector<FileEntry>& files, int selectedIndex, int x, int y, int maxItems);

    // Text utilities
    void PrintAt(int x, int y, const std::string& text);
    void PrintCentered(int y, const std::string& text, int width = 80);
    std::string TruncateText(const std::string& text, int maxLength) const;
    std::string FormatFileSize(long bytes) const;

private:
    GXRModeObj* videoMode;
    void* consoleBuffer;
    bool initialized;

    // Console dimensions
    int consoleWidth;
    int consoleHeight;

    // UI styling constants
    static const char BORDER_HORIZONTAL = '-';
    static const char BORDER_VERTICAL = '|';
    static const char BORDER_CORNER = '+';
    static const char SELECTION_MARKER = '>';

    void InitializeConsole();
    void DrawHorizontalLine(int x, int y, int length, char character = BORDER_HORIZONTAL);
    void DrawVerticalLine(int x, int y, int length, char character = BORDER_VERTICAL);
};

#endif // UI_H
