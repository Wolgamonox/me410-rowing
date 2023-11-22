#include <ezLED.h>
#include <SPI.h>
#include <ACAN2517FD.h>
#include <Moteus.h>

#include "src/button/button.h"
#include "src/communication/esp_now/follower/esp_now_follower.h"
#include "src/communication/esp_now/leader/esp_now_leader.h"
#include "src/communication/wired/follower/wired_follower.h"
#include "src/communication/wired/leader/wired_leader.h"

// PIN CONFIGURATION

// SPI
const int SPI_SCLK_PIN = GPIO_NUM_18;
const int SPI_MISO_PIN = GPIO_NUM_23;   // SDO output of MCP2517FD
const int SPI_MOSI_PIN = GPIO_NUM_19;    // SDI input of MCP2517FD

const int SPI_CS_PIN = GPIO_NUM_16;
const int SPI_INT_PIN = GPIO_NUM_17;

// ACAN2517FD
ACAN2517FD can(SPI_CS_PIN, SPI, SPI_INT_PIN);

// Moteus motor
Moteus moteus1(can, []() {
  Moteus::Options options;
  options.id = 1;
  return options;
}());

Moteus::PositionMode::Command position_cmd;
Moteus::PositionMode::Format position_fmt;

// Button
const int BUTTON_PIN = GPIO_NUM_2;

// LEDs
const int LEADER_LED_PIN = GPIO_NUM_25;
const int FOLLOWER_LED_PIN = GPIO_NUM_26;
const int CONNECTION_LED_PIN = GPIO_NUM_27;

// Encoder
const int ENCODER_PIN = GPIO_NUM_0;
float encoder_val = 0.;

// Override pin for communication
// If this pin is grounded, we will use wired communication instead of wireless
const int COMM_OVERRIDE_PIN = GPIO_NUM_32;

// I2C, not used in the code but mark here so we don't use the pins for
// something else
const int I2C_SDA_PIN = GPIO_NUM_21;
const int I2C_SCL_PIN = GPIO_NUM_22;


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

// Communication
LeaderComService* leaderCommunication;
FollowerComService* followerCommunication;

// Period at which we check if we have a connection
int beaconPeriod = 1000;
unsigned long startTime = 0;

ezLED leaderLED(LEADER_LED_PIN);
ezLED followerLED(FOLLOWER_LED_PIN);
ezLED bluetoothLED(CONNECTION_LED_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("Device started.");

  SPI.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

  button.setup();

  // ACAN2517FD setup
  ACAN2517FDSettings settings(ACAN2517FDSettings::OSC_20MHz, 1000 * 1000, DataBitRateFactor::x5);
  // settings.mRequestedMode = ACAN2517FDSettings::ExternalLoopBack; // not sure if we need to keep that
  settings.mArbitrationSJW = 2;
  settings.mDriverTransmitFIFOSize = 1;
  settings.mDriverReceiveFIFOSize = 2;

  const uint32_t errorCode = can.begin(settings, [] { can.isr(); }); // CAN error codes
  while (errorCode != 0) {
    Serial.print(F("CAN error 0x"));
    Serial.println(errorCode, HEX);
    delay(1000);
  }

  // Moteus setup
  moteus1.SetStop();
  Serial.println(F("all stopped"));

  position_fmt.velocity_limit = Moteus::kFloat;
  position_fmt.accel_limit = Moteus::kFloat;

  position_cmd.velocity_limit = 2.0;
  position_cmd.accel_limit = 3.0;

  // If pin is grounded, use wired communication
  pinMode(COMM_OVERRIDE_PIN, INPUT_PULLUP);

  // uncomment to use wired communication when you don't have a cable to ground
  // the pin
  //   pinMode(COMM_OVERRIDE_PIN, INPUT);

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
          leaderCommunication = new WiredLeader();
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
      encoder_val = analogRead(ENCODER_PIN);
      Serial.println("Leader angle value:");
      mainState.kneeFlexion = map(encoder_val, 0, 1023, 0, 1); // map to motor values between 0 and 1
      Serial.println(mainState.kneeFlexion);
      //mainState.kneeFlexion = random(0, 10000) / 100.0f;

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
        position_cmd.position = mainState.kneeFlexion;
        moteus1.SetPositionWaitComplete(position_cmd, 0.02, &position_fmt);
        // TODO: add print_state function from moteus code

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
