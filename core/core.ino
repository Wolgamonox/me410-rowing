#include <ACAN2517FD.h>
#include <Moteus.h>
#include <SPI.h>
#include <ezLED.h>

#include "src/button/button.h"
#include "src/buzzer.h"
#include "src/communication/esp_now/follower/esp_now_follower.h"
#include "src/communication/esp_now/leader/esp_now_leader.h"
#include "src/communication/wired/follower/wired_follower.h"
#include "src/communication/wired/leader/wired_leader.h"

// For definitions of debugPrint and debugPrintln check
#include "src/definitions.h"

// Upload on follower or leader
// Comment out to upload on leader
// #define UPLOAD_FOLLOWER

// PIN CONFIGURATION

// SPI
const int SPI_SCLK_PIN = GPIO_NUM_17;
const int SPI_MISO_PIN = GPIO_NUM_16;  // SDO output of MCP2517FD
const int SPI_MOSI_PIN = GPIO_NUM_4;   // SDI input of MCP2517FD

const int SPI_CS_PIN = GPIO_NUM_18;
const int SPI_INT_PIN = GPIO_NUM_21;

// Motor utils
static uint32_t gNextSendMillis = 0;
const int millisBetweenSends = 20;
const float POS_ERROR = 0.005;

// Button
const int BUTTON_PIN = GPIO_NUM_32;

// LEDs
const int STATUS_LED_PIN = GPIO_NUM_14;
const int CONNECTION_LED_PIN = GPIO_NUM_26;

// Encoder (for leader!)
const int ENCODER_PIN = GPIO_NUM_35;
int encoder_val = 0;

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

  // The minimum and maximum angle of the knee
  float minimumAngle = 0.0f;
  float maximumAngle = 1.0f;

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

  // The target flexion of the knee
  float targetKneeFlexion = 0.0f;
} mainState;

// Motor configuration
#ifdef UPLOAD_FOLLOWER
// ACAN2517FD
ACAN2517FD can(SPI_CS_PIN, SPI, SPI_INT_PIN);

// Moteus motor
Moteus moteus1(can, []() {
  Moteus::Options options;
  options.id = 2;

  options.disable_brs = true;
  options.query_format.position = Moteus::kFloat;
  return options;
}());

Moteus::PositionMode::Command position_cmd;
Moteus::PositionMode::Format position_fmt;
#endif

Button button(BUTTON_PIN);
ButtonState buttonState = ButtonState::none;

Buzzer buzzer(BUZZER_PIN);

// Communication
LeaderComService* leaderCommunication;
FollowerComService* followerCommunication;

// Period at which we check if we have a connection
int beaconPeriod = 1000;
unsigned long beaconTime = 0;

// Period at which we send messages
int commPeriod = 5;
unsigned long commTime = 0;

// Period at which we monitor the angle
int monitorPeriod = 30;
unsigned long monitorTime = 0;

ezLED statusLED(STATUS_LED_PIN);
ezLED connectionLED(CONNECTION_LED_PIN);

void setup() {
  Serial.begin(UART_BAUD_RATE);
  debugPrintln("Device started.");

  // print upload follower to avoid mistakes
#ifdef UPLOAD_FOLLOWER
  debugPrintln("UPLOAD_FOLLOWER defined");
#endif

  SPI.begin(SPI_SCLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

  button.setup();

  // Setup for encoder
  // set the resolution to 12 bits (0-4096)
  analogReadResolution(12);

#ifdef UPLOAD_FOLLOWER
  // ACAN2517FD setup
  ACAN2517FDSettings settings(ACAN2517FDSettings::OSC_40MHz, 1000 * 1000, DataBitRateFactor::x5);
  settings.mArbitrationSJW = 2;
  settings.mDriverTransmitFIFOSize = 1;
  settings.mDriverReceiveFIFOSize = 2;

  const uint32_t errorCode = can.begin(settings, [] { can.isr(); });  // CAN error codes
  while (errorCode != 0) {
    debugPrint(F("CAN error 0x"));
    debugPrintln(errorCode, HEX);
    delay(1000);
  }

  // Moteus setup
  moteus1.SetStop();
  debugPrintln(F("Motor stopped"));

  // First, in case this motor has been opened with tview, we will try
  // to stop any unsolicited data that may be occurring.
  moteus1.DiagnosticCommand(F("tel stop"));
  moteus1.SetDiagnosticFlush();

  position_fmt.velocity_limit = Moteus::kFloat;
  position_fmt.accel_limit = Moteus::kFloat;

  position_cmd.velocity_limit = 2.0;
  position_cmd.accel_limit = 3.0;

  moteus1.DiagnosticCommand(F("conf set servo.pid_position.kp 2.0"));
  moteus1.DiagnosticCommand(F("conf set servo.pid_position.ki 1.0"));
  moteus1.DiagnosticCommand(F("conf set servo.pid_position.kd 0.1"));

  const auto current_kp = moteus1.DiagnosticCommand(F("conf get servo.pid_position.kp"), Moteus::kExpectSingleLine);
  const auto current_ki = moteus1.DiagnosticCommand(F("conf get servo.pid_position.ki"), Moteus::kExpectSingleLine);
  const auto current_kd = moteus1.DiagnosticCommand(F("conf get servo.pid_position.kd"), Moteus::kExpectSingleLine);
  debugPrint("KP: ");
  debugPrint(current_kp);
  debugPrint("KI: ");
  debugPrint(current_ki);
  debugPrint(" KD: ");
  debugPrintln(current_kd);
#endif

  // If pin is grounded, use wired communication
  pinMode(COMM_OVERRIDE_PIN, INPUT_PULLUP);

  // uncomment to use wired communication when you don't have a cable to ground
  // the pin
  // pinMode(COMM_OVERRIDE_PIN, INPUT);

  statusLED.turnON();
  connectionLED.turnON();

  delay(1000);

  statusLED.turnOFF();
  connectionLED.turnOFF();

  debugPrintln("Setup done.");
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
    debugPrintln("Serial command received: " + command);
    // If button command received, set button state
    if (command[0] == 'B') {
      switch (command[1]) {
        case 'S':
          buttonState = ButtonState::shortPress;
          debugPrintln("Button short press");
          break;
        case 'L':
          buttonState = ButtonState::longPress;
          debugPrintln("Button long press");
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

        debugPrintln("Role set to leader");

        if (digitalRead(COMM_OVERRIDE_PIN) == LOW) {
          debugPrintln("Using wired communication");
          leaderCommunication = new WiredLeader();
        } else {
          debugPrintln("Using wireless communication");
          leaderCommunication = new EspNowLeader();
        }

        leaderCommunication->init();

      } else if (buttonState == ButtonState::shortPress) {
        mainState.role = MainState::Role::FOLLOWER;

        buzzer.followerTone();
        // Turn off led so that it can blink at a different rate
        // when we set the leds
        connectionLED.turnOFF();

        debugPrintln("Role set to follower");

        if (digitalRead(COMM_OVERRIDE_PIN) == LOW) {
          debugPrintln("Using wired communication");
          followerCommunication = new WiredFollower();
        } else {
          debugPrintln("Using wireless communication");
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
        if (millis() > beaconTime + beaconPeriod) {
          beaconTime = millis();

          if (leaderCommunication->isConnected()) {
            mainState.connected = true;
            buzzer.connectionTone();
            debugPrintln("Connected");
          }
        }

        // Do nothing else until connected
        break;
      }

      // Sense angle
      encoder_val = analogRead(ENCODER_PIN);
      // map to motor values between 0 and 1 and
      // invert them to account for the different rotation direction
      mainState.kneeFlexion = 1 - ((float)encoder_val / 4096.f);

      // Calibration: setting the minimum and maximum angle
      if (mainState.calibrationState != MainState::CalibrationState::DONE) {
        performCalibration();

        // Do nothing else until calibration is done
        break;
      }

      // Scale angle to the range [0, 1] with the minimum and maximum angle
      mainState.kneeFlexion = mapFloat(mainState.kneeFlexion, mainState.minimumAngle, mainState.maximumAngle, 0, 1);

      // SAFETY: If angle is not a number, set to 0 and stop controlling the motor
      if (isnan(mainState.kneeFlexion) && mainState.active) {
        mainState.kneeFlexion = 0;
        mainState.active = false;
        buzzer.deactivationTone();
        debugPrintln("Error: wrong angle, not sending");
      }

      // SAFETY: constrain received angle to the range [0, 1]
      mainState.kneeFlexion = constrain(mainState.kneeFlexion, 0, 1);

      // Update button state to start or stop sending angle
      if (buttonState == ButtonState::longPress) {
        if (!mainState.active) {
          mainState.active = true;
          buzzer.activationTone();
          debugPrintln("Sending angle");
        }
      } else if (buttonState == ButtonState::shortPress) {
        if (mainState.active) {
          mainState.active = false;
          buzzer.deactivationTone();
          debugPrintln("Not sending angle");
        }
      }

      if (mainState.active) {
        // Send angle at a fixed rate
        if (millis() > commTime + commPeriod) {
          commTime = millis();
          leaderCommunication->send(mainState.kneeFlexion);
        }

        debugPrint("Motor angle value: ");
        debugPrintln(mainState.kneeFlexion);

        // MONITOR: send angle through serial
        if (millis() > monitorTime + monitorPeriod) {
          monitorTime = millis();
          monitorPrintf("%f\n", mainState.kneeFlexion);
        }
      }

      break;

    case MainState::Role::FOLLOWER:
      // main loop

      // while not connected, wait for messages
      // when we get a message, consider ourselves connected
      if (!mainState.connected) {
        if (followerCommunication->isConnected()) {
          mainState.connected = true;
          buzzer.connectionTone();
          debugPrintln("Connected");
        }
        break;
      }

      // Calibration: setting the minimum and maximum angle
      if (mainState.calibrationState != MainState::CalibrationState::DONE) {
#ifdef UPLOAD_FOLLOWER
        delay(20);
        // Set motor to brake mode to have access to the position
        moteus1.SetBrake();

        // sense angle from motor
        const auto& v = moteus1.last_result().values;
        mainState.kneeFlexion = v.position;
#endif
        performCalibration();

        // Do nothing else until calibration is done
        break;
      }

      // Update button state to start or stop following angle
      if (buttonState == ButtonState::longPress) {
        if (!mainState.active) {
          mainState.active = true;
          buzzer.activationTone();
          debugPrintln("Following angle");
        }
      } else if (buttonState == ButtonState::shortPress) {
        if (mainState.active) {
          mainState.active = false;
          buzzer.deactivationTone();

#ifdef UPLOAD_FOLLOWER
          disableMotor();
#endif
          debugPrintln("Not following angle");
        }
      }

      // Receive angle from leader
      mainState.targetKneeFlexion = followerCommunication->getValue();

      // TODO: disable control when not receiving angle

      // SAFETY: If angle is not a number, set to 0 and stop controlling the motor
      if (isnan(mainState.targetKneeFlexion) && mainState.active) {
        mainState.targetKneeFlexion = 0;
        mainState.active = false;
        buzzer.deactivationTone();
        debugPrintln("Error: wrong angle, not following");
      }

      // SAFETY: constrain received angle to the range [0, 1]
      mainState.targetKneeFlexion = constrain(mainState.targetKneeFlexion, 0, 1);

      // Inversly map the angle to the motor values
      mainState.targetKneeFlexion = mapFloat(mainState.targetKneeFlexion, 0, 1, mainState.minimumAngle, mainState.maximumAngle);

#ifdef UPLOAD_FOLLOWER
      if (mainState.active) {
        const auto time = millis();

        if (gNextSendMillis < time) {  // send motor command every 20ms
          const auto& v = moteus1.last_result().values;
          mainState.kneeFlexion = v.position;

          // Might not be needed
          // if (abs(mainState.kneeFlexion - mainState.targetKneeFlexion) <= POS_ERROR) {  // deadzone threshold
          //   moteus1.DiagnosticCommand(F("conf set servo.pid_position.kp 0.01"));
          // } else {
          //   moteus1.DiagnosticCommand(F("conf set servo.pid_position.kp 2.5"));
          // }

          // set motor angle as setpoint for the motor controller
          // STRANGE: subtract 0.2 to account for the offset of the motor
          // TODO: solve this offset or at least get a more precise value
          position_cmd.position = mainState.targetKneeFlexion - 0.2;
          position_cmd.velocity = 1.;
          moteus1.SetPosition(position_cmd, &position_fmt);

          gNextSendMillis += millisBetweenSends;
        }

        // MONITOR: send angle through serial
        if (millis() > monitorTime + monitorPeriod) {
          monitorTime = millis();
          monitorPrintf("%f\n", mainState.kneeFlexion);
        }
      }
#endif
      break;
  }

  // set leds to reflect current status
  setLeds();
}

// Calibration
void performCalibration() {
  switch (mainState.calibrationState) {
    case MainState::CalibrationState::NOT_CALIBRATED:
      if (buttonState == ButtonState::longPress) {
        mainState.calibrationState = MainState::CalibrationState::WAIT_FOR_MINIMUM;
        buzzer.beep();

        debugPrintln("Calibration started: Go to minimum and press button.");
      }
      break;

    case MainState::CalibrationState::WAIT_FOR_MINIMUM:
      if (buttonState == ButtonState::shortPress) {
        mainState.calibrationState = MainState::CalibrationState::WAIT_FOR_MAXIMUM;
        mainState.minimumAngle = mainState.kneeFlexion;
        buzzer.beep();

        debugPrint(mainState.minimumAngle);
        debugPrintln("Minimum set. Go to maximum and press button.");
      }
      break;
    case MainState::CalibrationState::WAIT_FOR_MAXIMUM:
      if (buttonState == ButtonState::shortPress) {
        mainState.calibrationState = MainState::CalibrationState::DONE;
        mainState.maximumAngle = mainState.kneeFlexion;
        buzzer.followerTone();

        debugPrint(mainState.maximumAngle);
        debugPrintln("Maximum set. Calibration done.");
      }
      break;
  }
}

#ifdef UPLOAD_FOLLOWER
// Helper function to stop the motor
void disableMotor() {
  moteus1.SetStop();
  debugPrintln("Motor stopped");
}
#endif

// Set the leds to reflect the current state
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

// map a value from one range to another for float values
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
