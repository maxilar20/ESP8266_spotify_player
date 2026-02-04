/**
 * @file main.cpp
 * @brief ESP8266 Spotify Player with NFC Control
 *
 * Main application that combines NFC tag reading with Spotify playback control.
 * When an NFC tag containing a Spotify URI is detected, the corresponding
 * album/playlist is played on the configured Spotify device.
 *
 * Features:
 * - Web interface for device selection at http://<device-ip>/
 * - LED ring feedback for different states
 * - Sound reactive LED visualization
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#include "Config.h"
#include "SpotifyClient.h"
#include "NfcReader.h"
#include "LedController.h"
#include "WebServer.h"

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
WebServerController webServer(WEB_SERVER_PORT, spotify, leds);

// =============================================================================
// Timing Variables
// =============================================================================

unsigned long lastWifiCheck = 0;
bool wifiConnected = false;
bool spotifyConnected = false;

// =============================================================================
// Function Prototypes
// =============================================================================

void initializeWifi();
void checkWifiConnection();
void handleNfcCard();
void updateSoundReactive();
void initializeSpotify();

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
    leds.showWifiConnecting();

    initializeWifi();

    if (wifiConnected) {
        initializeSpotify();

        // Start web server
        webServer.begin();
        DEBUG_PRINT(F("Web interface available at: http://"));
        DEBUG_PRINTLN(WiFi.localIP());
    }

    DEBUG_PRINTLN(F("Initializing NFC reader..."));
    if (nfcReader.begin()) {
        DEBUG_PRINTLN(F("NFC reader initialized successfully"));
    } else {
        DEBUG_PRINTLN(F("Warning: NFC reader initialization failed"));
        leds.showTagFailure();
        delay(2000);
    }

    // Show idle state when ready
    if (wifiConnected && spotifyConnected) {
        leds.showIdle();
    }

    DEBUG_PRINTLN(F("Setup complete. Ready to scan NFC tags."));
}

// =============================================================================
// Main Loop
// =============================================================================

void loop() {
    checkWifiConnection();

    // Handle web server requests
    if (wifiConnected) {
        webServer.handleClient();
    }

    // Update LEDs
    if (leds.getState() == LedState::IDLE) {
        updateSoundReactive();
    }
    leds.update();

    // Check for NFC cards
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

    // Custom callback during config portal
    wifiManager.setAPCallback([](WiFiManager* mgr) {
        DEBUG_PRINTLN(F("Entered config portal mode"));
        // Could show special LED pattern here
    });

    if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
        DEBUG_PRINTLN(F("Failed to connect and hit timeout"));
        leds.showWifiError();
        delay(3000);
        ESP.restart();
    }

    wifiConnected = true;
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
        wifiConnected = false;
        DEBUG_PRINTLN(F("WiFi disconnected, attempting reconnection..."));
        leds.showWifiError();
        WiFi.reconnect();

        // Give it a few seconds to reconnect
        delay(5000);

        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            DEBUG_PRINTLN(F("WiFi reconnected!"));
            DEBUG_PRINT(F("IP: "));
            DEBUG_PRINTLN(WiFi.localIP());

            // Restore appropriate state
            if (spotifyConnected) {
                leds.showIdle();
            } else {
                leds.showSpotifyError();
            }
        }
    } else if (!wifiConnected) {
        // Recovered from disconnection
        wifiConnected = true;
    }
}

// =============================================================================
// Spotify Functions
// =============================================================================

void initializeSpotify() {
    DEBUG_PRINTLN(F("Initializing Spotify client..."));
    leds.showSpotifyConnecting();

    if (spotify.begin()) {
        spotifyConnected = true;
        DEBUG_PRINTLN(F("Spotify client initialized successfully"));
        leds.showTagSuccess();
    } else {
        spotifyConnected = false;
        DEBUG_PRINTLN(F("Warning: Spotify client initialization incomplete"));
        DEBUG_PRINTLN(F("Use the web interface to select a device"));
        leds.showSpotifyError();
        delay(2000);
    }
}

// =============================================================================
// NFC Functions
// =============================================================================

void handleNfcCard() {
    DEBUG_PRINTLN(F("NFC card detected, reading..."));
    leds.showNfcReading();

    NfcReadResult result = nfcReader.readSpotifyUri();

    if (result.success) {
        DEBUG_PRINT(F("Playing: "));
        DEBUG_PRINTLN(result.spotifyUri);

        // Check if we have a device selected
        if (!spotify.isDeviceAvailable()) {
            DEBUG_PRINTLN(F("No device selected. Use web interface to select a device."));
            leds.showSpotifyError();
            delay(2000);
            leds.showIdle();
            return;
        }

        // Check if we're authenticated
        if (!spotify.isAuthenticated()) {
            DEBUG_PRINTLN(F("Not authenticated with Spotify"));
            leds.showSpotifyError();

            // Try to refresh token
            if (spotify.refreshToken()) {
                DEBUG_PRINTLN(F("Token refreshed, retrying..."));
            } else {
                delay(2000);
                leds.showIdle();
                return;
            }
        }

        if (spotify.playUri(result.spotifyUri)) {
            DEBUG_PRINTLN(F("Playback started successfully!"));
            leds.showTagSuccess();
        } else {
            DEBUG_PRINTLN(F("Failed to start playback"));
            leds.showTagFailure();
        }
    } else {
        DEBUG_PRINT(F("NFC read failed: "));
        DEBUG_PRINTLN(result.errorMessage);
        leds.showTagFailure();
    }
}

// =============================================================================
// Sound Reactive Functions
// =============================================================================

void updateSoundReactive() {
    int audioLevel = analogRead(MIC_PIN);
    leds.updateSoundReactive(audioLevel);
}
