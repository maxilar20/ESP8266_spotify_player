/**
 * @file WiFiSetup.cpp
 * @brief WiFi Setup implementation using WiFiManager
 *
 * This module is isolated to avoid header conflicts between
 * WiFiManager (ESP8266WebServer) and ESPAsyncWebServer.
 */

#include "WiFiSetup.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "Config.h"

bool initializeWiFi(const char* apName, const char* apPassword, uint16_t timeoutSeconds) {
    DEBUG_PRINTLN(F("Starting WiFi configuration..."));

    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(timeoutSeconds);

    // Custom callback during config portal
    wifiManager.setAPCallback([](WiFiManager* mgr) {
        DEBUG_PRINTLN(F("Entered config portal mode"));
    });

    if (!wifiManager.autoConnect(apName, apPassword)) {
        DEBUG_PRINTLN(F("Failed to connect and hit timeout"));
        return false;
    }

    DEBUG_PRINTLN(F("WiFi connected"));
    DEBUG_PRINT(F("IP address: "));
    DEBUG_PRINTLN(WiFi.localIP());

    return true;
}

void resetWiFiSettings() {
    DEBUG_PRINTLN(F("Resetting WiFi settings..."));
    WiFi.disconnect(true);
}
