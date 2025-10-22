/**
 * Unity Test Framework Configuration for ESP32 Embedded Tests
 */

#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Configure Unity for ESP32
#define UNITY_OUTPUT_CHAR(a) Serial.print(a)
#define UNITY_OUTPUT_FLUSH() Serial.flush()
#define UNITY_OUTPUT_START() Serial.begin(115200)
#define UNITY_OUTPUT_COMPLETE() Serial.end()

#endif // UNITY_CONFIG_H