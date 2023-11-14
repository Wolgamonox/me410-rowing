#include "button.h"

Button::Button(int buttonPin) : buttonObject(buttonPin) {
}

void Button::setup() {
    buttonObject.setDebounceTime(50);
}

void Button::loop() {
    buttonObject.loop();

    buttonState = ButtonState::none;

    if (buttonObject.isPressed()) {
        pressedTime = millis();
        isPressing = true;
        isLongDetected = false;
    }

    if (buttonObject.isReleased()) {
        isPressing = false;
        releasedTime = millis();

        long pressDuration = releasedTime - pressedTime;

        if (pressDuration < SHORT_PRESS_TIME) {
            buttonState = ButtonState::shortPress;
        }
    }

    if (isPressing == true && isLongDetected == false) {
        long pressDuration = millis() - pressedTime;

        if (pressDuration > LONG_PRESS_TIME) {
            buttonState = ButtonState::longPress;
            isLongDetected = true;
        }
    }
}
