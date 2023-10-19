#ifndef BUTTON_H
#define BUTTON_H

#include <ezButton.h>

enum class ButtonState {
    NONE,
    SHORT_PRESS,
    TRIPLE_PRESS,
    LONG_PRESS,
}

// Press times in milliseconds
const int SHORT_PRESS_TIME = 2000;
const int LONG_PRESS_TIME = 2000;
const int TRIPLE_PRESS_TIME = 500;

const int BUTTON_PIN = 2;

class Button {
   private:
    // ezButton object
    ezButton buttonObject = ezButton(BUTTON_PIN);

    // Used to determine the button state (short press, long press, none)
    unsigned long pressedTime = 0;
    unsigned long releasedTime = 0;
    bool isPressing = false;
    bool isLongDetected = false;

    ButtonState buttonState = ButtonState::NONE;

   public:
    void setup();
    void loop();

    buttonState getButtonState();

    Button();
    ~Button();
}

#endif  // BUTTON_H