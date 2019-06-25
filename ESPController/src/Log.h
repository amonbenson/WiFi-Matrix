#ifndef LOG_H
#define LOG_H

#include "Settings.h"

#ifdef SERIAL_DEBUG

// Print, Println and Printf functions for debugging
#define DEBUG(STR) Serial.print(STR);
#define DEBUGLN(STR) Serial.println(STR);
#define DEBUGF(STR, ...) Serial.printf(STR, __VA_ARGS__);

// Warn and datal error functions
#define WARN(STR) DEBUGF("WARNING: %s\n", STR);
#define FATAL(STR) DEBUGF("FATAL ERROR: %s --- RESET\n", STR); \
    ESP.restart();

#else

#define DEBUG(STR)
#define DEBUGLN(STR)
#define DEBUGF(STR, ...)

#define WARN(STR)
#define FATAL(STR) ESP.restart();

#endif

#endif