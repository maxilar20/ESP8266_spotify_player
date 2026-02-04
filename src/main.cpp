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
#include "ConfigManager.h"

// =============================================================================
// Global Objects
// =============================================================================

LedController leds(LED_PIN, NUM_LEDS);
NfcReader nfcReader(NFC_SS_PIN, NFC_RST_PIN);
SpotifyClient spotify;
ConfigManager configManager;

// =============================================================================
// State Variables
// =============================================================================

unsigned long lastWifiCheck = 0;
unsigned long wifiDisconnectedSince = 0;
int wifiReconnectAttempts = 0;
bool spotifyInitialized = false;

// =============================================================================
// Function Prototypes
// =============================================================================

void initializeWifi();
void checkWifiConnection();
void resetWifiSettings();
void handleNfcCard();
void updateSoundReactive();
void initializeSpotify();
void printConfigInstructions();

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

    // Initialize WiFi reset button if configured
    if (WIFI_RESET_BUTTON_PIN >= 0) {
        pinMode(WIFI_RESET_BUTTON_PIN, INPUT_PULLUP);
    }

    // Initialize LED controller
    leds.begin();
    leds.showConnecting();

    // Initialize WiFi
    initializeWifi();

    // Initialize configuration manager
    DEBUG_PRINTLN(F("Initializing configuration manager..."));
    if (!configManager.begin()) {
        DEBUG_PRINTLN(F("Warning: Config manager initialization failed"));
    }

    // Start web configuration server
    configManager.startWebServer(80);
    printConfigInstructions();

    // Try to initialize Spotify if configured
    initializeSpotify();

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
    // Check for WiFi reset button press (hold for 3 seconds)
    if (WIFI_RESET_BUTTON_PIN >= 0) {
        static unsigned long buttonPressStart = 0;
        if (digitalRead(WIFI_RESET_BUTTON_PIN) == LOW) {
            if (buttonPressStart == 0) {
                buttonPressStart = millis();
            } else if (millis() - buttonPressStart > 3000) {
                DEBUG_PRINTLN(F("WiFi reset button pressed - resetting settings..."));
                resetWifiSettings();
                buttonPressStart = 0;
            }
        } else {
            buttonPressStart = 0;
        }
    }

    // Handle web configuration requests
    configManager.handleClient();

    // Check WiFi connection periodically
    checkWifiConnection();

    // Update sound reactive LEDs
    updateSoundReactive();

    // Re-initialize Spotify if configuration changed
    if (configManager.isConfigured() && !spotifyInitialized) {
        initializeSpotify();
    }

    // Check for NFC cards (only if Spotify is ready)
    if (spotifyInitialized && nfcReader.isNewCardPresent()) {
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
    wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT);

    // Set custom parameters to show on config page
    wifiManager.setCustomHeadElement("<style>body{background:#1DB954;}</style>");

    // Try to connect, or start AP for configuration
    if (!wifiManager.autoConnect(WIFI_AP_NAME, WIFI_AP_PASSWORD)) {
        DEBUG_PRINTLN(F("Failed to connect and hit timeout"));
        DEBUG_PRINTLN(F("Restarting to retry..."));
        delay(3000);
        ESP.restart();
    }

    DEBUG_PRINTLN(F("WiFi connected"));
    DEBUG_PRINT(F("IP address: "));
    DEBUG_PRINTLN(WiFi.localIP());

    // Reset reconnection tracking
    wifiReconnectAttempts = 0;
    wifiDisconnectedSince = 0;
}

/**
 * @brief Check and maintain WiFi connection with smart recovery
 */
void checkWifiConnection() {
    unsigned long currentTime = millis();

    if (WiFi.status() != WL_CONNECTED) {
        // Track when disconnection started
        if (wifiDisconnectedSince == 0) {
            wifiDisconnectedSince = currentTime;
            DEBUG_PRINTLN(F("WiFi disconnected!"));
            leds.showError();
        }

        unsigned long disconnectedDuration = currentTime - wifiDisconnectedSince;

        // If disconnected for too long (60 seconds), restart WiFi portal
        if (disconnectedDuration > 60000) {
            DEBUG_PRINTLN(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
            DEBUG_PRINTLN(F("⚠ WiFi disconnected for >60 seconds"));
            DEBUG_PRINTLN(F("⟳ Restarting WiFi configuration..."));
            DEBUG_PRINTLN(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
            resetWifiSettings();
            return;
        }

        // Only attempt reconnection at intervals
        if (currentTime - lastWifiCheck < WIFI_RECONNECT_DELAY) {
            return;
        }

        lastWifiCheck = currentTime;
        wifiReconnectAttempts++;

        // After max attempts, restart portal immediately
        if (wifiReconnectAttempts > WIFI_MAX_RECONNECT_ATTEMPTS) {
            DEBUG_PRINTLN(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
            DEBUG_PRINT(F("✗ Failed "));
            DEBUG_PRINT(WIFI_MAX_RECONNECT_ATTEMPTS);
            DEBUG_PRINTLN(F(" reconnection attempts"));
            DEBUG_PRINTLN(F("⟳ Restarting WiFi configuration..."));
            DEBUG_PRINTLN(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
            resetWifiSettings();
            return;
        }

        // Show status
        DEBUG_PRINT(F("Reconnect attempt "));
        DEBUG_PRINT(wifiReconnectAttempts);
        DEBUG_PRINT(F("/"));
        DEBUG_PRINT(WIFI_MAX_RECONNECT_ATTEMPTS);
        DEBUG_PRINT(F(" (disconnected for "));
        DEBUG_PRINT(disconnectedDuration / 1000);
        DEBUG_PRINTLN(F("s)..."));

        // Non-blocking reconnection - just trigger it
        WiFi.reconnect();

    } else {
        // Connected - reset tracking
        if (wifiDisconnectedSince != 0) {
            DEBUG_PRINTLN(F("✓ WiFi connection restored!"));
            leds.showSuccess();
            delay(500);
        }
        wifiReconnectAttempts = 0;
        wifiDisconnectedSince = 0;
        lastWifiCheck = currentTime;
    }
}

/**
 * @brief Reset WiFi settings and restart configuration portal
 */
void resetWifiSettings() {
    DEBUG_PRINTLN(F("Resetting WiFi settings..."));

    WiFiManager wifiManager;
    wifiManager.resetSettings();

    DEBUG_PRINTLN(F("WiFi settings cleared. Restarting..."));
    delay(1000);
    ESP.restart();
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

// =============================================================================
// Spotify Initialization
// =============================================================================

/**
 * @brief Initialize or reinitialize Spotify client with current config
 */
void initializeSpotify() {
    if (!configManager.isConfigured()) {
        DEBUG_PRINTLN(F("Spotify not configured. Visit the web interface to set up."));
        spotifyInitialized = false;
        return;
    }

    DEBUG_PRINTLN(F("Initializing Spotify client..."));

    SpotifyConfig config = configManager.getConfig();
    spotify.setCredentials(
        config.clientId,
        config.clientSecret,
        config.deviceName,
        config.refreshToken
    );

    if (spotify.begin()) {
        DEBUG_PRINTLN(F("Spotify client initialized successfully"));
        leds.showSuccess();
        spotifyInitialized = true;
    } else {
        DEBUG_PRINTLN(F("Warning: Spotify client initialization incomplete"));
        leds.showError();
        delay(2000);
        leds.showConnecting();
        spotifyInitialized = false;
    }
}

/**
 * @brief Print configuration instructions to serial
 */
void printConfigInstructions() {
    Serial.println();
    Serial.println(F("╔════════════════════════════════════════════════╗"));
    Serial.println(F("║         WEB CONFIGURATION AVAILABLE            ║"));
    Serial.println(F("╠════════════════════════════════════════════════╣"));
    Serial.print(F("║  Open in browser: http://"));
    Serial.print(WiFi.localIP());
    String padding = "";
    for (int i = WiFi.localIP().toString().length(); i < 15; i++) {
        padding += " ";
    }
    Serial.print(padding);
    Serial.println(F("  ║"));
    Serial.println(F("║                                                ║"));
    Serial.println(F("║  Configure Spotify credentials via the web     ║"));
    Serial.println(F("║  interface - no need to hardcode anything!     ║"));
    Serial.println(F("╚════════════════════════════════════════════════╝"));
    Serial.println();

    if (configManager.isConfigured()) {
        Serial.println(F("✓ Spotify is configured and ready"));
    } else {
        Serial.println(F("⚠ Spotify NOT configured - visit web interface"));
    }
    Serial.println();
}
