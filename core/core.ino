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

    float kneeFlexion = 0.0f;

    bool following = false;

} mainState;

LeaderBLEHandler* leaderBLEHandler = NULL;
FollowerBLEHandler* followerBLEHandler = NULL;

void setup() {
    Serial.begin(115200);
    Serial.println("Device started");
}

void loop() {
    // DEBUG: simulate button presses with serial commands
    // "BS" = button short press
    // "BL" = button long press
    // "B3" = button triple presses

    ButtonState buttonState = ButtonState::NONE;
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
                case '3':
                    buttonState = ButtonState::TRIPLE_PRESS;
                    Serial.println("Button triple press");
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
            }

            // TODO: Add calibration step

            // main loop

            if (leaderBLEHandler->isConnected()) {
                // check if we have a button press to stop sending angle
                if (buttonState == ButtonState::SHORT_PRESS) {
                    if (leaderBLEHandler->getSendingKneeFlexion()) {
                        leaderBLEHandler->setSendingKneeFlexion(false);
                    }
                } else if (buttonState == ButtonState::LONG_PRESS) {
                    if (!leaderBLEHandler->getSendingKneeFlexion()) {
                        leaderBLEHandler->setSendingKneeFlexion(true);
                    }
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
            }

            // TODO: Add calibration step

            // main loop

            if (mainState.connectionStepDone) {
                followerBLEHandler->loop();

                if (followerBLEHandler->isConnected()) {
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
                    delay(100);
                }
            }

            break;
    }
}
