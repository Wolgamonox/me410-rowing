#include <ezLED.h>

#include "src/button/button.h"
#include "src/communication/esp_now/follower/esp_now_follower.h"
#include "src/communication/esp_now/leader/esp_now_leader.h"

// PIN CONFIGURATION

// SPI
const int SPI_MISO_PIN = GPIO_NUM_19;
const int SPI_MOSI_PIN = GPIO_NUM_23;
const int SPI_SCLK_PIN = GPIO_NUM_18;
const int SPI_SS_PIN = GPIO_NUM_5;

// Button
const int BUTTON_PIN = GPIO_NUM_4;

// LEDs
const int LEADER_LED_PIN = GPIO_NUM_25;
const int FOLLOWER_LED_PIN = GPIO_NUM_26;
const int CONNECTION_LED_PIN = GPIO_NUM_27;

// Override pin for communication
// If this pin is grounded, we will use wired communication instead of wireless
const int OVERRIDE_PIN = GPIO_NUM_32;

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

    // For the leader, active means sending the angle to the follower
    // For the follower, active means controlling the motor to follow the angle from the leader
    bool active = false;

    // If the leader has at least one follower connected
    // If the follower is connected to a leader
    bool connected = false;

    // The current flexion of the knee
    // This does not correspond to the angle of the knee, but a value between 0 and 1
    // representing the flexion of the knee on a scaled range
    float kneeFlexion = 0.0f;
} mainState;

Button button(BUTTON_PIN);
ButtonState buttonState = ButtonState::none;

// Communication
LeaderComService* leaderCommunication;
FollowerComService* followerCommunication;

ezLED leaderLED(GPIO_NUM_18);
ezLED followerLED(GPIO_NUM_19);
ezLED bluetoothLED(GPIO_NUM_21);

void setup() {
    Serial.begin(115200);
    Serial.println("Device started.");

    button.setup();

    leaderLED.turnON();
    followerLED.turnON();
    bluetoothLED.turnON();

    delay(1000);

    leaderLED.turnOFF();
    followerLED.turnOFF();
    bluetoothLED.turnOFF();

    Serial.println("Setup done.");
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
                    buttonState = ButtonState::shortPress;
                    Serial.println("Button short press");
                    break;
                case 'L':
                    buttonState = ButtonState::longPress;
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
            if (buttonState == ButtonState::longPress) {
                mainState.role = MainState::Role::LEADER;

                Serial.println("Role set to leader");

                leaderCommunication = new EspNowLeader(mainState.kneeFlexion);

                if (!leaderCommunication->init()) {
                    Serial.println("Failed to init leader communication");
                } else {
                    Serial.println("Leader communication init done");
                }

                if (!leaderCommunication->attach()) {
                    Serial.println("Failed to attach peer");
                } else {
                    Serial.println("Successfully attached peer");
                }

            } else if (buttonState == ButtonState::shortPress) {
                mainState.role = MainState::Role::FOLLOWER;
                Serial.println("Role set to follower");
            }
            break;
        case MainState::Role::LEADER:
            // TODO: Add calibration step

            // main loop

            mainState.kneeFlexion = random(0, 10000) / 100.0f;

            leaderCommunication->notify();

            delay(2000);

            // if (mainState.connected) {
            //     // check if we have a button press to stop sending angle
            //     if (buttonState == ButtonState::shortPress) {
            //         mainState.following = false;

            //     } else if (buttonState == ButtonState::longPress) {
            //         mainState.following = true;
            //     }

            //     // Sense angle
            //     // Debug: random knee flexion between 0 and 100
            //     mainState.kneeFlexion = random(0, 10000) / 100.0f;

            //     // Send angle
            //     if (mainState.connectionStepDone) {
            //         // leaderBLEHandler->loop();
            //     }
            // }

            break;

        case MainState::Role::FOLLOWER:
            // TODO: Add calibration step

            // main loop

            // if (mainState.connectionStepDone) {
            //     // followerBLEHandler->loop();

            //     // mainState.connected = followerBLEHandler->isConnected();
            //     if (mainState.connected) {
            //         if (buttonState == ButtonState::longPress) {
            //             if (!mainState.following) {
            //                 mainState.following = true;
            //                 Serial.println("Following");
            //             }
            //         } else if (buttonState == ButtonState::shortPress) {
            //             if (mainState.following) {
            //                 mainState.following = false;
            //                 Serial.println("Not following");
            //             }
            //         }
            //     }

            //     if (mainState.following) {
            //         // if (fabs(mainState.kneeFlexion - followerBLEHandler->getKneeFlexion()) > 0.001f) {
            //         //     mainState.kneeFlexion = followerBLEHandler->getKneeFlexion();

            //         //     Serial.println("Knee flexion: " + String(mainState.kneeFlexion));
            //         // }

            //         // delay for debugging
            //         // delay(100);
            //     }
            // }

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
            if (mainState.active) {
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
            if (mainState.active) {
                if (followerLED.getState() != LED_BLINKING) {
                    followerLED.blink(300, 300);
                }
            } else {
                followerLED.turnON();
            }
            break;
    }
}
