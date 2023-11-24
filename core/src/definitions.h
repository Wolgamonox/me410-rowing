// Uncomment to enable debug prints
#define DEBUG

#ifdef DEBUG
#define UART_BAUD_RATE 115200
#define debugPrint(...) Serial.print(__VA_ARGS__)
#define debugPrintf(...) Serial.printf(__VA_ARGS__)
#define debugPrintln(...) Serial.println(__VA_ARGS__)
#else
#define UART_BAUD_RATE 9600
#define debugPrint(...)
#define debugPrintf(...)
#define debugPrintln(...)
#endif

// Uncomment to enable angle monitoring through serial
#define MONITOR_ANGLE

#ifdef MONITOR_ANGLE
#define MONITOR_ANGLE_PERIOD 500  // in ms
#define monitorPrintf(...) Serial.printf(__VA_ARGS__)
#else
#define monitorPrintf(...)
#endif
