/**
 * @file main.cpp
 * @brief ESP8266 Spotify Player with NFC Control
 *
 * Main application that combines NFC tag reading with Spotify playback control.
 * When an NFC tag containing a Spotify URI is detected, the corresponding
 * album/playlist is played on the configured Spotify device.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#include "Config.h"
#include "SpotifyClient.h"
#include "NfcReader.h"
#include "LedController.h"

// =============================================================================
// Global Objects
// =============================================================================

LedController leds(LED_PIN, NUM_LEDS);
NfcReader nfcReader(NFC_SS_PIN, NFC_RST_PIN);
SpotifyClient spotify(
    SPOTIFY_CLIENT_ID,
    SPOTIFY_CLIENT_SECRET,
    SPOTIFY_DEVICE_NAME,
    SPOTIFY_REFRESH_TOKEN
);

// =============================================================================
// Timing Variables
// =============================================================================

unsigned long lastWifiCheck = 0;

// =============================================================================
// Function Prototypes
// =============================================================================

void initializeWifi();
void checkWifiConnection();
void handleNfcCard();
void updateSoundReactive();

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("================================="));
    Serial.println(F("ESP8266 Spotify Player v2.0"));
    Serial.println(F("================================="));

    pinMode(MIC_PIN, INPUT);

    leds.begin();
    leds.showConnecting();

    initializeWifi();

    DEBUG_PRINTLN(F("Initializing Spotify client..."));
    if (spotify.begin()) {
        DEBUG_PRINTLN(F("Spotify client initialized successfully"));
        leds.showSuccess();
    } else {
        DEBUG_PRINTLN(F("Warning: Spotify client initialization incomplete"));
        leds.showError();
        delay(2000);
        leds.showConnecting();
    }

    DEBUG_PRINTLN(F("Initializing NFC reader..."));
    if (nfcReader.begin()) {
        DEBUG_PRINTLN(F("NFC reader initialized successfully"));
        leds.showSuccess();
    } else {
        DEBUG_PRINTLN(F("Warning: NFC reader initialization failed"));
        leds.showError();
    }

    DEBUG_PRINTLN(F("Setup complete. Ready to scan NFC tags."));
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
    checkWifiConnection();
    updateSoundReactive();

    if (nfcReader.isNewCardPresent()) {
        handleNfcCard();
    }
}

// =============================================================================
// WiFi Functions
// =============================================================================

void initializeWifi() {
    DEBUG_PRINTLN(F("Starting WiFi configuration..."));

    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(180);

    if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
        DEBUG_PRINTLN(F("Failed to connect and hit timeout"));
        delay(3000);
        ESP.restart();
    }

    DEBUG_PRINTLN(F("WiFi connected"));
    DEBUG_PRINT(F("IP address: "));
    DEBUG_PRINTLN(WiFi.localIP());
}

void checkWifiConnection() {
    unsigned long currentTime = millis();

    if (currentTime - lastWifiCheck < WIFI_RECONNECT_DELAY) {
        return;
    }

    lastWifiCheck = currentTime;

    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN(F("WiFi disconnected, attempting reconnection..."));
        WiFi.reconnect();

        // Give it a few seconds to reconnect
        delay(5000);

        if (WiFi.status() == WL_CONNECTED) {
            DEBUG_PRINTLN(F("WiFi reconnected!"));
            DEBUG_PRINT(F("IP: "));
            DEBUG_PRINTLN(WiFi.localIP());
        }
    }
}

// =============================================================================
// NFC Functions
// =============================================================================

void handleNfcCard() {
    leds.showReading();

    NfcReadResult result = nfcReader.readSpotifyUri();

    if (result.success) {
        DEBUG_PRINT(F("Playing: "));
        DEBUG_PRINTLN(result.spotifyUri);

        if (spotify.playUri(result.spotifyUri)) {
            leds.showSuccess();
        } else {
            DEBUG_PRINTLN(F("Failed to start playback"));
            leds.showError();
            delay(1000);
            leds.showSuccess();
        }
    } else {
        DEBUG_PRINT(F("NFC read failed: "));
        DEBUG_PRINTLN(result.errorMessage);
        leds.showError();
        delay(1000);
        leds.showSuccess();
    }
}

// =============================================================================
// Sound Reactive Functions
// =============================================================================

void updateSoundReactive() {
    int audioLevel = analogRead(MIC_PIN);
    leds.updateSoundReactive(audioLevel);
    leds.update();
}
