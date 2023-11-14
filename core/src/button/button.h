#ifndef BUTTON_H
#define BUTTON_H

#include <ezButton.h>

enum class ButtonState {
    none,
    shortPress,
    longPress,
};

// Press times in milliseconds
const int SHORT_PRESS_TIME = 2000;
const int LONG_PRESS_TIME = 2000;

class Button {
   public:
    void setup();
    void loop();

    ButtonState getButtonState() { return buttonState; };

    Button(int buttonPin);

   private:
    // ezButton object
    ezButton buttonObject;

    // Used to determine the button state (short press, long press, none)
    unsigned long pressedTime = 0;
    unsigned long releasedTime = 0;
    bool isPressing = false;
    bool isLongDetected = false;

    ButtonState buttonState = ButtonState::none;
};

#endif  // BUTTON_H