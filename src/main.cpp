/**
 * @file main.cpp
 * @brief ESP8266 Spotify Player with NFC Control
 *
 * Main application that combines NFC tag reading with Spotify playback control.
 * When an NFC tag containing a Spotify URI is detected, the corresponding
 * album/playlist is played on the configured Spotify device.
 *
 * Features:
 * - WiFi configuration via captive portal (WiFiManager)
 * - NFC tag reading for Spotify URIs
 * - Spotify Web API integration for playback control
 * - LED status indication
 * - Sound reactive LED visualization
 *
 * @author ESP8266 Spotify Player Project
 * @version 2.0.0
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
    // Initialize serial communication
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("================================="));
    Serial.println(F("ESP8266 Spotify Player v2.0"));
    Serial.println(F("================================="));

    // Initialize microphone input
    pinMode(MIC_PIN, INPUT);

    // Initialize LED controller
    leds.begin();
    leds.showConnecting();

    // Initialize WiFi
    initializeWifi();

    // Initialize Spotify client
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

    // Initialize NFC reader
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
    // Check WiFi connection periodically
    checkWifiConnection();

    // Update sound reactive LEDs
    updateSoundReactive();

    // Check for NFC cards
    if (nfcReader.isNewCardPresent()) {
        handleNfcCard();
    }
}

// =============================================================================
// WiFi Functions
// =============================================================================

/**
 * @brief Initialize WiFi using WiFiManager captive portal
 */
void initializeWifi() {
    DEBUG_PRINTLN(F("Starting WiFi configuration..."));

    WiFiManager wifiManager;

    // Set timeout for configuration portal (seconds)
    wifiManager.setConfigPortalTimeout(180);

    // Try to connect, or start AP for configuration
    if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
        DEBUG_PRINTLN(F("Failed to connect and hit timeout"));
        // Reset and try again
        delay(3000);
        ESP.restart();
    }

    DEBUG_PRINTLN(F("WiFi connected"));
    DEBUG_PRINT(F("IP address: "));
    DEBUG_PRINTLN(WiFi.localIP());
}

/**
 * @brief Check and maintain WiFi connection
 */
void checkWifiConnection() {
    unsigned long currentTime = millis();

    if (currentTime - lastWifiCheck < WIFI_RECONNECT_DELAY) {
        return;
    }

    lastWifiCheck = currentTime;

    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN(F("WiFi disconnected, attempting reconnection..."));

        WiFi.disconnect();
        WiFi.reconnect();

        // Wait a bit for reconnection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            DEBUG_PRINT(F("."));
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            DEBUG_PRINTLN(F("\nReconnected to WiFi"));
        } else {
            DEBUG_PRINTLN(F("\nReconnection failed"));
        }
    }
}

// =============================================================================
// NFC Functions
// =============================================================================

/**
 * @brief Handle detected NFC card
 */
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

/**
 * @brief Update sound reactive LED visualization
 */
void updateSoundReactive() {
    int audioLevel = analogRead(MIC_PIN);
    leds.updateSoundReactive(audioLevel);
    leds.update();
}
