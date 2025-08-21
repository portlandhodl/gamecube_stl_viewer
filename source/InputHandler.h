#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <gccore.h>

/**
 * Input state structure to track button presses and analog inputs
 */
struct InputState {
    // Button states
    bool upPressed;
    bool downPressed;
    bool leftPressed;
    bool rightPressed;
    bool aPressed;
    bool bPressed;
    bool startPressed;
    bool zPressed;
    bool lTriggerHeld;
    bool rTriggerHeld;

    // Analog stick values
    s8 stickX;
    s8 stickY;
    s8 cStickX;
    s8 cStickY;

    InputState() { Clear(); }

    void Clear() {
        upPressed = downPressed = leftPressed = rightPressed = false;
        aPressed = bPressed = startPressed = zPressed = false;
        lTriggerHeld = rTriggerHeld = false;
        stickX = stickY = cStickX = cStickY = 0;
    }
};

/**
 * Input handling class for GameCube controller
 */
class InputHandler {
public:
    InputHandler();
    ~InputHandler();

    void Initialize();
    void Update();

    const InputState& GetCurrentState() const { return currentState; }

    // Convenience methods
    bool IsMenuNavigationInput() const;
    bool IsExitRequested() const { return currentState.startPressed; }
    bool IsSelectPressed() const { return currentState.aPressed; }
    bool IsBackPressed() const { return currentState.bPressed; }

    // Camera control helpers
    bool HasCameraRotationInput() const;
    void GetCameraRotationDelta(f32& deltaX, f32& deltaY) const;
    f32 GetZoomDelta() const;

private:
    InputState currentState;

    static const s8 STICK_DEADZONE = 10;
    static const f32 ROTATION_SENSITIVITY;
    static const f32 FINE_ROTATION_SENSITIVITY;
    static const f32 ZOOM_SPEED;
    static const f32 FAST_ZOOM_SPEED;
};

#endif // INPUT_HANDLER_H
