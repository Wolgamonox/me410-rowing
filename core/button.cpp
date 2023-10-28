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
            // Triple press detected
            if (numberOfShortPresses == 2) {
                buttonState = ButtonState::TRIPLE_PRESS;
                numberOfShortPresses = 0;
                return;
            }

            // First short press detected
            if (numberOfShortPresses == 0) {
                firstShortPressTime = millis();
                numberOfShortPresses++;
            } else if (numberOfShortPresses > 0) {
                // If delay between presses is to long, reset the counter
                if (millis() - firstShortPressTime > TRIPLE_PRESS_TIME) {
                    // if we only pressed once set the state to short press
                    if (numberOfShortPresses == 1) {
                        buttonState = ButtonState::SHORT_PRESS;
                    }
                    numberOfShortPresses = 0;
                } else {
                    numberOfShortPresses++;
                }
            }
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
