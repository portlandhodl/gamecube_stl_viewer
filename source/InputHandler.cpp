#include "InputHandler.h"
#include <cmath>

// Static constants
const f32 InputHandler::ROTATION_SENSITIVITY = 0.002f;
const f32 InputHandler::FINE_ROTATION_SENSITIVITY = 0.001f;
const f32 InputHandler::ZOOM_SPEED = 2.0f;
const f32 InputHandler::FAST_ZOOM_SPEED = 4.0f;

InputHandler::InputHandler() {
}

InputHandler::~InputHandler() {
}

void InputHandler::Initialize() {
    PAD_Init();
    currentState.Clear();
}

void InputHandler::Update() {
    PAD_ScanPads();

    u32 pressed = PAD_ButtonsDown(0);
    u32 held = PAD_ButtonsHeld(0);

    // Update button states
    currentState.upPressed = (pressed & PAD_BUTTON_UP) != 0;
    currentState.downPressed = (pressed & PAD_BUTTON_DOWN) != 0;
    currentState.leftPressed = (pressed & PAD_BUTTON_LEFT) != 0;
    currentState.rightPressed = (pressed & PAD_BUTTON_RIGHT) != 0;
    currentState.aPressed = (pressed & PAD_BUTTON_A) != 0;
    currentState.bPressed = (pressed & PAD_BUTTON_B) != 0;
    currentState.startPressed = (pressed & PAD_BUTTON_START) != 0;
    currentState.zPressed = (pressed & PAD_TRIGGER_Z) != 0;
    currentState.lTriggerHeld = (held & PAD_TRIGGER_L) != 0;
    currentState.rTriggerHeld = (held & PAD_TRIGGER_R) != 0;

    // Update analog stick values
    currentState.stickX = PAD_StickX(0);
    currentState.stickY = PAD_StickY(0);
    currentState.cStickX = PAD_SubStickX(0);
    currentState.cStickY = PAD_SubStickY(0);
}

bool InputHandler::IsMenuNavigationInput() const {
    return currentState.upPressed || currentState.downPressed ||
           currentState.leftPressed || currentState.rightPressed;
}

bool InputHandler::HasCameraRotationInput() const {
    return (abs(currentState.stickX) > STICK_DEADZONE) ||
           (abs(currentState.stickY) > STICK_DEADZONE) ||
           (abs(currentState.cStickX) > STICK_DEADZONE) ||
           (abs(currentState.cStickY) > STICK_DEADZONE) ||
           currentState.leftPressed || currentState.rightPressed ||
           currentState.upPressed || currentState.downPressed;
}

void InputHandler::GetCameraRotationDelta(f32& deltaX, f32& deltaY) const {
    deltaX = 0.0f;
    deltaY = 0.0f;

    // Main analog stick for primary rotation
    if (abs(currentState.stickX) > STICK_DEADZONE) {
        deltaY += static_cast<f32>(currentState.stickX) * ROTATION_SENSITIVITY;
    }
    if (abs(currentState.stickY) > STICK_DEADZONE) {
        deltaX += static_cast<f32>(currentState.stickY) * ROTATION_SENSITIVITY;
    }

    // C-stick for fine rotation control
    if (abs(currentState.cStickX) > STICK_DEADZONE) {
        deltaY += static_cast<f32>(currentState.cStickX) * FINE_ROTATION_SENSITIVITY;
    }
    if (abs(currentState.cStickY) > STICK_DEADZONE) {
        deltaX += static_cast<f32>(currentState.cStickY) * FINE_ROTATION_SENSITIVITY * 0.5f;
    }

    // D-pad for precise adjustment
    if (currentState.leftPressed) deltaY -= 0.02f;
    if (currentState.rightPressed) deltaY += 0.02f;
    if (currentState.upPressed) deltaX -= 0.015f;
    if (currentState.downPressed) deltaX += 0.015f;
}

f32 InputHandler::GetZoomDelta() const {
    f32 delta = 0.0f;

    if (currentState.lTriggerHeld) {
        delta -= ZOOM_SPEED;
    }
    if (currentState.rTriggerHeld) {
        delta += ZOOM_SPEED;
    }
    if (currentState.zPressed) {
        delta -= FAST_ZOOM_SPEED;
    }

    return delta;
}
