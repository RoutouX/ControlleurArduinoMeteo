#pragma once
#include <Arduino.h>

// 0 = errors, 1 = warnings, 2 = info, 3 = debug
#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif

inline void logLine_(const char* tag, const char* msg) {
  Serial.print("[");
  Serial.print(tag);
  Serial.print("] ");
  Serial.println(msg);
}

inline void logLine_(const char* tag, const String& msg) {
  Serial.print("[");
  Serial.print(tag);
  Serial.print("] ");
  Serial.println(msg);
}

#if LOG_LEVEL >= 2
inline void logInfo(const char* tag, const char* msg) { logLine_(tag, msg); }
inline void logInfo(const char* tag, const String& msg) { logLine_(tag, msg); }
#else
inline void logInfo(const char* tag, const char* msg) { (void)tag; (void)msg; }
inline void logInfo(const char* tag, const String& msg) { (void)tag; (void)msg; }
#endif

#if LOG_LEVEL >= 1
inline void logWarn(const char* tag, const char* msg) { logLine_(tag, msg); }
inline void logWarn(const char* tag, const String& msg) { logLine_(tag, msg); }
#else
inline void logWarn(const char* tag, const char* msg) { (void)tag; (void)msg; }
inline void logWarn(const char* tag, const String& msg) { (void)tag; (void)msg; }
#endif

inline void logError(const char* tag, const char* msg) { logLine_(tag, msg); }
inline void logError(const char* tag, const String& msg) { logLine_(tag, msg); }

#if LOG_LEVEL >= 3
inline void logDebug(const char* tag, const char* msg) { logLine_(tag, msg); }
inline void logDebug(const char* tag, const String& msg) { logLine_(tag, msg); }
#else
inline void logDebug(const char* tag, const char* msg) { (void)tag; (void)msg; }
inline void logDebug(const char* tag, const String& msg) { (void)tag; (void)msg; }
#endif

