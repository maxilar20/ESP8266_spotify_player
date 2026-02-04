/**
 * @file WiFiSetup.h
 * @brief WiFi Setup module using WiFiManager
 *
 * This module is isolated to avoid header conflicts between
 * WiFiManager (ESP8266WebServer) and ESPAsyncWebServer.
 */

#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <Arduino.h>

/**
 * @brief Initialize WiFi using WiFiManager
 * @param apName Access point name for config portal
 * @param apPassword Access point password
 * @param timeoutSeconds Config portal timeout
 * @return true if connected successfully
 */
bool initializeWiFi(const char* apName, const char* apPassword, uint16_t timeoutSeconds);

/**
 * @brief Reset WiFi settings and restart
 */
void resetWiFiSettings();

#endif // WIFI_SETUP_H
