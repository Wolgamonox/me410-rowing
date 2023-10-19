

// main state of the device
struct {
    bool isLeader;

    enum {
        NOT_CALIBRATED,
        WAIT_FOR_MINIMUM,
        WAIT_FOR_MAXIMUM,
        DONE,
    } calibrationState = NOT_CALIBRATED;

    // // If leader, use this state
    // enum {
    //     IDLE,
    //     SEARCHING,
    // } connectionState = IDLE;

    // // If follower, use this state
    // enum {
    //     NOT_CONNECTED,
    //     SEARCHING,
    //     CONNECTED,
    // } leaderConnectionState = NOT_CONNECTED;

} mainState;

void setup() {
}

void loop() {

    int buttonState = 0;

    // Check a button state to react on events

    if {}

}
