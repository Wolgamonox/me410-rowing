// Uncomment to enable debug prints
// debug 1 means normal debug prints
// debug 2 means verbose debug prints
#define DEBUG 1

#define UART_BAUD_RATE 115200

#ifdef DEBUG
#define debugPrint(...) Serial.print(__VA_ARGS__)
#define debugPrintf(...) Serial.printf(__VA_ARGS__)
#define debugPrintln(...) Serial.println(__VA_ARGS__)
#else
#define debugPrint(...)
#define debugPrintf(...)
#define debugPrintln(...)
#endif

// Uncomment to enable angle monitoring through serial
#define MONITOR_ANGLE

#ifdef MONITOR_ANGLE
#define monitorPrintf(...) Serial.printf(__VA_ARGS__)
#else
#define monitorPrintf(...)
#endif
