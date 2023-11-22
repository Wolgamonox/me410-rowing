#include <SPI.h>
#include <ezLED.h>

#include "src/button/button.h"
#include "src/buzzer.h"
#include "src/communication/esp_now/follower/esp_now_follower.h"
#include "src/communication/esp_now/leader/esp_now_leader.h"
#include "src/communication/wired/follower/wired_follower.h"
#include "src/communication/wired/leader/wired_leader.h"

#define DEBUG

#ifdef DEBUG
#define debugPrint(x) Serial.println(x)
#else
#define debugPrint(x)
#endif

// PIN CONFIGURATION

// SPI
const int SPI_SCLK_PIN = GPIO_NUM_17;
const int SPI_MISO_PIN = GPIO_NUM_16;  // SDO output of MCP2517FD
const int SPI_MOSI_PIN = GPIO_NUM_4;   // SDI input of MCP2517FD

const int SPI_CS_PIN = GPIO_NUM_18;
const int SPI_INT_PIN = GPIO_NUM_21;

// Button
const int BUTTON_PIN = GPIO_NUM_32;

// LEDs
const int STATUS_LED_PIN = GPIO_NUM_14;
const int CONNECTION_LED_PIN = GPIO_NUM_26;

// Buzzer
const int BUZZER_PIN = GPIO_NUM_13;

// Override pin for communication
// If this pin is grounded, we will use wired communication instead of wireless
const int COMM_OVERRIDE_PIN = GPIO_NUM_19;

// I2C, not used in the code but mark here so we don't use the pins for
// something else
const int I2C_SDA_PIN = GPIO_NUM_22;
const int I2C_SCL_PIN = GPIO_NUM_23;

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
  // For the follower, active means controlling the motor to follow the angle
  // from the leader
  bool active = false;

  // If the leader has at least one follower connected
  // If the follower is connected to a leader
  bool connected = false;

  // The current flexion of the knee
  // This does not correspond to the angle of the knee, but a value between 0
  // and 1 representing the flexion of the knee on a scaled range
  float kneeFlexion = 0.0f;
} mainState;

Button button(BUTTON_PIN);
ButtonState buttonState = ButtonState::none;

Buzzer buzzer(BUZZER_PIN);

// Communication
LeaderComService* leaderCommunication;
FollowerComService* followerCommunication;

// Period at which we check if we have a connection
int beaconPeriod = 1000;
unsigned long startTime = 0;

ezLED statusLED(STATUS_LED_PIN);
ezLED connectionLED(CONNECTION_LED_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("Device started.");

  SPI.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

  button.setup();

  // If pin is grounded, use wired communication
  pinMode(COMM_OVERRIDE_PIN, INPUT_PULLUP);

  // uncomment to use wired communication when you don't have a cable to ground
  // the pin
  //   pinMode(COMM_OVERRIDE_PIN, INPUT);

  statusLED.turnON();
  connectionLED.turnON();

  delay(1000);

  statusLED.turnOFF();
  connectionLED.turnOFF();

  Serial.println("Setup done.");
}

void loop() {
  button.loop();

  statusLED.loop();
  connectionLED.loop();

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

        buzzer.leaderTone();

        // Turn off led so that it can blink at a different rate
        // when we set the leds
        connectionLED.turnOFF();

        Serial.println("Role set to leader");

        if (digitalRead(COMM_OVERRIDE_PIN) == LOW) {
          Serial.println("Using wired communication");
          leaderCommunication = new WiredLeader();
        } else {
          Serial.println("Using wireless communication");
          leaderCommunication = new EspNowLeader();
        }

        leaderCommunication->init();

      } else if (buttonState == ButtonState::shortPress) {
        mainState.role = MainState::Role::FOLLOWER;

        buzzer.followerTone();
        // Turn off led so that it can blink at a different rate
        // when we set the leds
        connectionLED.turnOFF();

        Serial.println("Role set to follower");

        if (digitalRead(COMM_OVERRIDE_PIN) == LOW) {
          Serial.println("Using wired communication");
          followerCommunication = new WiredFollower();
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
        // Try to connect periodically
        if (millis() > startTime + beaconPeriod) {
          startTime = millis();

          if (leaderCommunication->isConnected()) {
            mainState.connected = true;
            buzzer.connectionTone();
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
          buzzer.connectionTone();
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

      break;
  }

  // set leds to reflect current status
  setLeds();
}

void setLeds() {
  if (mainState.connected) {
    connectionLED.turnON();
  } else {
    switch (mainState.role) {
      case MainState::Role::NOT_SET:
        if (connectionLED.getState() != LED_BLINKING) {
          connectionLED.blink(1000, 1000);
        }
        break;
      case MainState::Role::LEADER:
      case MainState::Role::FOLLOWER:
        if (connectionLED.getState() != LED_BLINKING) {
          connectionLED.blink(250, 250);
        }
        break;
    }
  }

  switch (mainState.role) {
    case MainState::Role::NOT_SET:
      if (statusLED.getState() != LED_BLINKING) {
        statusLED.blink(1000, 1000);
      }
      break;
    case MainState::Role::LEADER:
      if (mainState.active) {
        if (statusLED.getState() != LED_BLINKING) {
          statusLED.blink(250, 250);
        }
      } else {
        statusLED.turnON();
      }
      break;
    case MainState::Role::FOLLOWER:
      if (mainState.active) {
        if (statusLED.getState() != LED_BLINKING) {
          statusLED.blink(250, 250);
        }
      } else {
        statusLED.turnOFF();
      }
      break;
  }
}
