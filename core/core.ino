#include <ezLED.h>

#include "button.h"
#include "follower_ble_handler.h"
#include "leader_ble_handler.h"

// Main state of the device
struct MainState {
    enum Role {
        NOT_SET,
        LEADER,
        FOLLOWER,
    } role = NOT_SET;

    enum CalibrationState {
        NOT_CALIBRATED,
        WAIT_FOR_MINIMUM,
        WAIT_FOR_MAXIMUM,
        DONE,
    } calibrationState = NOT_CALIBRATED;

    bool connectionStepDone = false;
    bool connected = false;

    float kneeFlexion = 0.0f;

    bool following = false;

} mainState;

LeaderBLEHandler* leaderBLEHandler = NULL;
FollowerBLEHandler* followerBLEHandler = NULL;

Button button;
ButtonState buttonState = ButtonState::NONE;

ezLED leaderLED(GPIO_NUM_18);
ezLED followerLED(GPIO_NUM_19);
ezLED bluetoothLED(GPIO_NUM_21);

void setup() {
    Serial.begin(115200);
    Serial.println("Device started");

    button.setup();

    leaderLED.turnON();
    followerLED.turnON();
    bluetoothLED.turnON();

    delay(1000);

    leaderLED.turnOFF();
    followerLED.turnOFF();
    bluetoothLED.turnOFF();

    Serial.println("Setup done");
}

void loop() {
    button.loop();

    leaderLED.loop();
    followerLED.loop();
    bluetoothLED.loop();

    buttonState = button.getButtonState();

    // DEBUG: simulate button presses with serial commands
    // "BS" = button short press
    // "BL" = button long press
    // "B3" = button triple presses
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        Serial.print("Serial command received: " + command);
        Serial.println();
        // If button command received, set button state
        if (command[0] == 'B') {
            switch (command[1]) {
                case 'S':
                    buttonState = ButtonState::SHORT_PRESS;
                    Serial.println("Button short press");
                    break;
                case 'L':
                    buttonState = ButtonState::LONG_PRESS;
                    Serial.println("Button long press");
                    break;

                default:
                    break;
            }
        }
    }

    switch (mainState.role) {
        case MainState::Role::NOT_SET:
            // If role not set, set role depending on button press
            if (buttonState == ButtonState::LONG_PRESS) {
                mainState.role = MainState::Role::LEADER;
                Serial.println("Role set to leader");
            } else if (buttonState == ButtonState::SHORT_PRESS) {
                mainState.role = MainState::Role::FOLLOWER;
                Serial.println("Role set to follower");
            }
            break;
        case MainState::Role::LEADER:
            // Bluetooth handling

            // If not connected to follower, start advertising
            if (!mainState.connectionStepDone) {
                leaderBLEHandler = new LeaderBLEHandler();
                leaderBLEHandler->setup();

                mainState.connectionStepDone = true;
                break;
            }

            // TODO: Add calibration step

            // main loop

            mainState.connected = leaderBLEHandler->isConnected();

            if (mainState.connected) {
                // check if we have a button press to stop sending angle
                if (buttonState == ButtonState::SHORT_PRESS) {
                    leaderBLEHandler->setSendingKneeFlexion(false);
                    mainState.following = false;

                } else if (buttonState == ButtonState::LONG_PRESS) {
                    leaderBLEHandler->setSendingKneeFlexion(true);
                    mainState.following = true;
                }

                // Sense angle
                // Debug: random knee flexion between 0 and 100
                mainState.kneeFlexion = random(0, 10000) / 100.0f;

                // Send angle
                if (mainState.connectionStepDone) {
                    leaderBLEHandler->setKneeFlexion(mainState.kneeFlexion);
                    leaderBLEHandler->loop();
                }
            }

            break;

        case MainState::Role::FOLLOWER:
            // If follower, do follower stuff

            // If not connected to leader, start scanning
            if (!mainState.connectionStepDone) {
                followerBLEHandler = new FollowerBLEHandler();
                followerBLEHandler->setup();

                mainState.connectionStepDone = true;
                break;
            }

            // TODO: Add calibration step

            // main loop

            if (mainState.connectionStepDone) {
                followerBLEHandler->loop();

                mainState.connected = followerBLEHandler->isConnected();
                if (mainState.connected) {
                    if (buttonState == ButtonState::LONG_PRESS) {
                        if (!mainState.following) {
                            mainState.following = true;
                            Serial.println("Following");
                        }
                    } else if (buttonState == ButtonState::SHORT_PRESS) {
                        if (mainState.following) {
                            mainState.following = false;
                            Serial.println("Not following");
                        }
                    }
                }

                if (mainState.following) {
                    if (fabs(mainState.kneeFlexion - followerBLEHandler->getKneeFlexion()) > 0.001f) {
                        mainState.kneeFlexion = followerBLEHandler->getKneeFlexion();

                        Serial.println("Knee flexion: " + String(mainState.kneeFlexion));
                    }

                    // delay for debugging
                    // delay(100);
                }
            }

            break;
    }

    // set leds to reflect current status
    setLeds();
}

void setLeds() {
    if (mainState.connected) {
        bluetoothLED.turnON();
    } else {
        switch (mainState.role) {
            case MainState::Role::NOT_SET:
                bluetoothLED.turnOFF();
                break;
            case MainState::Role::LEADER:
            case MainState::Role::FOLLOWER:
                if (bluetoothLED.getState() != LED_BLINKING) {
                    bluetoothLED.blink(500, 500);
                }
                break;
        }
    }
    switch (mainState.role) {
        case MainState::Role::NOT_SET:
            leaderLED.turnOFF();
            followerLED.turnOFF();
            break;
        case MainState::Role::LEADER:
            if (mainState.following) {
                if (leaderLED.getState() != LED_BLINKING) {
                    leaderLED.blink(300, 300);
                }
            } else {
                leaderLED.turnON();
            }

            followerLED.turnOFF();
            break;
        case MainState::Role::FOLLOWER:
            leaderLED.turnOFF();
            if (mainState.following) {
                if (followerLED.getState() != LED_BLINKING) {
                    followerLED.blink(300, 300);
                }
            } else {
                followerLED.turnON();
            }
            break;
    }
}
