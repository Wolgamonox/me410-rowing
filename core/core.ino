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
const int COMM_OVERRIDE_PIN = GPIO_NUM_32;

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

// Period at which we check if we have a connection
int beaconPeriod = 1000;
unsigned long startTime = 0;

ezLED leaderLED(GPIO_NUM_18);
ezLED followerLED(GPIO_NUM_19);
ezLED bluetoothLED(GPIO_NUM_21);

void setup() {
    Serial.begin(115200);
    Serial.println("Device started.");

    button.setup();

    // If pin is grounded, use wired communication
    pinMode(COMM_OVERRIDE_PIN, INPUT_PULLUP);

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

                if (digitalRead(COMM_OVERRIDE_PIN) == LOW) {
                    Serial.println("Using wired communication");
                    // leaderCommunication = new LeaderComService();
                } else {
                    Serial.println("Using wireless communication");
                    leaderCommunication = new EspNowLeader();
                }

                leaderCommunication->init();

            } else if (buttonState == ButtonState::shortPress) {
                mainState.role = MainState::Role::FOLLOWER;
                Serial.println("Role set to follower");

                if (digitalRead(COMM_OVERRIDE_PIN) == LOW) {
                    Serial.println("Using wired communication");
                    // followerCommunication = new FollowerComService();
                } else {
                    Serial.println("Using wireless communication");
                    followerCommunication = new EspNowFollower();
                }

                followerCommunication->init();
            }
            break;
        case MainState::Role::LEADER:
            // main loop

            // while not connected, send messages until we get a response
            if (!mainState.connected) {
                if (millis() > startTime + beaconPeriod) {
                    startTime = millis();

                    if (leaderCommunication->isConnected()) {
                        mainState.connected = true;
                        Serial.println("Connected");
                    }
                }

                // Do nothing else until connected
                break;
            }

            // TODO: Add calibration step

            // Update button state to start or stop sending angle
            if (buttonState == ButtonState::longPress) {
                if (!mainState.active) {
                    mainState.active = true;
                    Serial.println("Sending angle");
                }
            } else if (buttonState == ButtonState::shortPress) {
                if (mainState.active) {
                    mainState.active = false;
                    Serial.println("Not sending angle");
                }
            }

            // Sense angle
            // TODO: relace with actual angle sensor and scaling function
            // DEBUG: fake angle
            mainState.kneeFlexion = random(0, 10000) / 100.0f;

            if (mainState.active) {
                // Send angle
                leaderCommunication->send(mainState.kneeFlexion);
            }

            // DEBUG: delay for sending messages
            delay(2000);

            break;

        case MainState::Role::FOLLOWER:
            // main loop

            // while not connected, wait for messages
            // when we get a message, consider ourselves connected
            if (!mainState.connected) {
                if (followerCommunication->isConnected()) {
                    mainState.connected = true;
                    Serial.println("Connected");
                }
                break;
            }

            // TODO: Add calibration step

            // Update button state to start or stop following angle
            if (buttonState == ButtonState::longPress) {
                if (!mainState.active) {
                    mainState.active = true;
                    Serial.println("Following angle");
                }
            } else if (buttonState == ButtonState::shortPress) {
                if (mainState.active) {
                    mainState.active = false;
                    Serial.println("Not following angle");
                }
            }

            // Receive angle from leader
            mainState.kneeFlexion = followerCommunication->getValue();

            if (mainState.active) {
                // set motor angle as setpoint for the motor controller

                // TODO: add safety checks on the value send to the motor

                // DEBUG: print received angle
                Serial.println("Angle setpoint: " + String(mainState.kneeFlexion));
            }

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
