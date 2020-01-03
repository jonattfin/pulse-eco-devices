#include <limits.h>

//Status LED pin
#define STATUS_LED_PIN D0

//Development / production profiles
//#define NO_CONNECTION_PROFILE 1
#define DEBUG_PROFILE 1

#ifdef DEBUG_PROFILE
  #define NUM_MEASURE_SESSIONS 10
  #define CYCLE_DELAY 8000
#else
  #define NUM_MEASURE_SESSIONS 30
  #define CYCLE_DELAY 30000
#endif


#define SH_DEBUG_PRINTLN(a) Serial.println(a)
#define SH_DEBUG_PRINT(a) Serial.print(a)
#define SH_DEBUG_PRINT_DEC(a,b) Serial.print(a,b)
#define SH_DEBUG_PRINTLN_DEC(a,b) Serial.println(a,b)