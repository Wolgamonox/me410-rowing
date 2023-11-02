#include "button.h"

Button::Button() {
}

Button::~Button() {
}

void Button::setup() {
    buttonObject.setDebounceTime(50);
}

void Button::loop() {
    buttonObject.loop();

    buttonState = ButtonState::NONE;

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
            buttonState = ButtonState::SHORT_PRESS;
        }
    }

    if (isPressing == true && isLongDetected == false) {
        long pressDuration = millis() - pressedTime;

        if (pressDuration > LONG_PRESS_TIME) {
            buttonState = ButtonState::LONG_PRESS;
            isLongDetected = true;
        }
    }
}
