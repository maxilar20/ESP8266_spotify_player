/**
 * @file main.cpp
 * @brief ESP8266 Spotify Player with NFC Control
 *
 * Main application that combines NFC tag reading with Spotify playback control.
 * When an NFC tag containing a Spotify URI is detected, the corresponding
 * album/playlist is played on the configured Spotify device.
 *
 * Features:
 * - Non-blocking state machine architecture
 * - Async web interface with WebSocket support
 * - Interrupt-driven NFC detection (optional)
 * - LED ring feedback for different states
 * - Sound reactive LED visualization
 * - Exponential backoff for API calls
 *
 * Refactored for real-time responsiveness with no blocking delays.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

#include "Config.h"
#include "SpotifyClient.h"
#include "NfcReader.h"
#include "LedController.h"
#include "WebServer.h"
#include "WiFiSetup.h"

// =============================================================================
// Application States (Non-blocking State Machine)
// =============================================================================

enum class AppState
{
    BOOT,                   // Initial boot state
    WIFI_CONNECTING,        // Connecting to WiFi
    WIFI_CONFIG_PORTAL,     // WiFi config portal active
    WIFI_CONNECTED,         // WiFi connected, initializing
    SPOTIFY_INITIALIZING,   // Initializing Spotify client
    IDLE,                   // Ready and waiting for NFC tags
    NFC_DETECTED,           // NFC card detected
    NFC_READING,            // Reading NFC tag data
    NFC_PROCESSING,         // Processing tag and calling Spotify
    PLAYBACK_SUCCESS,       // Playback started successfully
    PLAYBACK_FAILED,        // Playback failed
    ERROR_RECOVERY,         // Recovering from errors
    WIFI_RECONNECTING       // Reconnecting to WiFi
};

// =============================================================================
// Global Objects
// =============================================================================

LedController leds(LED_PIN, NUM_LEDS);
NfcReader nfcReader(NFC_SS_PIN, NFC_RST_PIN, NFC_IRQ_PIN);
SpotifyClient spotify(
    SPOTIFY_CLIENT_ID,
    SPOTIFY_CLIENT_SECRET,
    SPOTIFY_DEVICE_NAME,
    SPOTIFY_REFRESH_TOKEN
);
WebServerController webServer(WEB_SERVER_PORT, spotify, leds);

// Ticker for periodic tasks
Ticker wifiCheckTicker;
Ticker ledUpdateTicker;

// =============================================================================
// State Machine Variables
// =============================================================================

AppState currentState = AppState::BOOT;
AppState previousState = AppState::BOOT;

// Non-blocking timing variables
unsigned long stateEntryTime = 0;
unsigned long lastStateUpdate = 0;
unsigned long lastNfcCheck = 0;
unsigned long lastSoundSample = 0;

// Status flags
bool wifiConnected = false;
bool spotifyConnected = false;
bool pendingRestart = false;
bool wifiCheckPending = false;

// Current NFC read result (stored between states)
String currentNfcUri;

// =============================================================================
// Function Prototypes
// =============================================================================

void changeState(AppState newState);
void updateStateMachine();
void handleBootState();
void handleWifiConnecting();
void handleWifiConnected();
void handleSpotifyInitializing();
void handleIdleState();
void handleNfcDetected();
void handleNfcReading();
void handleNfcProcessing();
void handlePlaybackSuccess();
void handlePlaybackFailed();
void handleErrorRecovery();
void handleWifiReconnecting();
void checkWifiStatus();
void updateSoundReactive();
const char* stateToString(AppState state);

// =============================================================================
// Ticker Callbacks
// =============================================================================

void IRAM_ATTR onWifiCheck() {
    wifiCheckPending = true;
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("================================="));
    Serial.println(F("ESP8266 Spotify Player v3.0"));
    Serial.println(F("Non-blocking Async Architecture"));
    Serial.println(F("================================="));

    // Initialize hardware
    pinMode(MIC_PIN, INPUT);

    // Initialize LED controller
    leds.begin();

    // Start in boot state
    changeState(AppState::BOOT);

    // Setup periodic WiFi check (non-blocking via ticker)
    wifiCheckTicker.attach_ms(WIFI_RECONNECT_DELAY, onWifiCheck);
}

// =============================================================================
// Main Loop (Non-blocking)
// =============================================================================

void loop() {
    // Update state machine
    updateStateMachine();

    // Update LEDs (non-blocking animations)
    leds.update();

    // Handle sound reactive in IDLE state
    if (currentState == AppState::IDLE) {
        unsigned long now = millis();
        if (now - lastSoundSample >= MIC_SAMPLE_INTERVAL) {
            lastSoundSample = now;
            updateSoundReactive();
        }
    }

    // Handle web server requests (synchronous server)
    if (wifiConnected) {
        webServer.handleClient();
    }

    // Check WiFi status periodically (triggered by ticker)
    if (wifiCheckPending && currentState == AppState::IDLE) {
        wifiCheckPending = false;
        checkWifiStatus();
    }

    // Handle pending restart (scheduled from web interface)
    if (pendingRestart) {
        pendingRestart = false;
        ESP.restart();
    }

    // Allow ESP8266 to handle background tasks
    yield();
}

// =============================================================================
// State Machine Implementation
// =============================================================================

void changeState(AppState newState) {
    if (newState == currentState) return;

    previousState = currentState;
    currentState = newState;
    stateEntryTime = millis();

    DEBUG_PRINT(F("State: "));
    DEBUG_PRINT(stateToString(previousState));
    DEBUG_PRINT(F(" -> "));
    DEBUG_PRINTLN(stateToString(newState));
}

void updateStateMachine() {
    unsigned long now = millis();

    // Rate limit state updates
    if (now - lastStateUpdate < STATE_MACHINE_INTERVAL) {
        return;
    }
    lastStateUpdate = now;

    switch (currentState) {
        case AppState::BOOT:
            handleBootState();
            break;

        case AppState::WIFI_CONNECTING:
            handleWifiConnecting();
            break;

        case AppState::WIFI_CONFIG_PORTAL:
            // WiFiManager handles this blocking - unavoidable
            break;

        case AppState::WIFI_CONNECTED:
            handleWifiConnected();
            break;

        case AppState::SPOTIFY_INITIALIZING:
            handleSpotifyInitializing();
            break;

        case AppState::IDLE:
            handleIdleState();
            break;

        case AppState::NFC_DETECTED:
            handleNfcDetected();
            break;

        case AppState::NFC_READING:
            handleNfcReading();
            break;

        case AppState::NFC_PROCESSING:
            handleNfcProcessing();
            break;

        case AppState::PLAYBACK_SUCCESS:
            handlePlaybackSuccess();
            break;

        case AppState::PLAYBACK_FAILED:
            handlePlaybackFailed();
            break;

        case AppState::ERROR_RECOVERY:
            handleErrorRecovery();
            break;

        case AppState::WIFI_RECONNECTING:
            handleWifiReconnecting();
            break;
    }
}

// =============================================================================
// State Handlers
// =============================================================================

void handleBootState() {
    // Show startup animation
    leds.showStartup();

    // Brief startup animation (non-blocking check)
    if (millis() - stateEntryTime >= 1500) {
        changeState(AppState::WIFI_CONNECTING);
    }
}

void handleWifiConnecting() {
    static bool wifiManagerStarted = false;

    if (!wifiManagerStarted) {
        leds.showWifiConnecting();

        // Use isolated WiFiSetup module to avoid header conflicts
        if (!initializeWiFi(WIFI_AP_NAME, WIFI_AP_PASSWORD, 180)) {
            DEBUG_PRINTLN(F("WiFi connection failed"));
            leds.showWifiError();
            changeState(AppState::ERROR_RECOVERY);
            wifiManagerStarted = false;
            return;
        }

        wifiConnected = true;
        wifiManagerStarted = false;
        changeState(AppState::WIFI_CONNECTED);
    }
}

void handleWifiConnected() {
    DEBUG_PRINTLN(F("WiFi connected"));
    DEBUG_PRINT(F("IP address: "));
    DEBUG_PRINTLN(WiFi.localIP());

    // Initialize NFC reader
    DEBUG_PRINTLN(F("Initializing NFC reader..."));
    if (!nfcReader.begin()) {
        DEBUG_PRINTLN(F("Warning: NFC reader initialization failed"));
        leds.showTagFailure();
        // Continue anyway - NFC might work later
    } else {
        DEBUG_PRINTLN(F("NFC reader initialized successfully"));
        if (nfcReader.isInterruptMode()) {
            DEBUG_PRINTLN(F("NFC interrupt mode active"));
        }
    }

    // Start web server (async - non-blocking)
    webServer.begin();
    DEBUG_PRINT(F("Web interface available at: http://"));
    DEBUG_PRINTLN(WiFi.localIP());

    changeState(AppState::SPOTIFY_INITIALIZING);
}

void handleSpotifyInitializing() {
    static bool initStarted = false;
    static unsigned long initStartTime = 0;

    if (!initStarted) {
        initStarted = true;
        initStartTime = millis();
        leds.showSpotifyConnecting();
        DEBUG_PRINTLN(F("Initializing Spotify client..."));
    }

    // Check for timeout
    if (millis() - initStartTime > SPOTIFY_INIT_TIMEOUT) {
        DEBUG_PRINTLN(F("Spotify initialization timeout"));
        spotifyConnected = false;
        initStarted = false;
        changeState(AppState::IDLE);
        webServer.notifyError("Spotify initialization timeout");
        return;
    }

    // Try to initialize (this may take time due to network)
    if (spotify.begin()) {
        spotifyConnected = true;
        DEBUG_PRINTLN(F("Spotify client initialized successfully"));
        leds.showTagSuccess();
        initStarted = false;
        changeState(AppState::IDLE);
        webServer.notifyStatusChange();
    } else {
        // If credentials are missing, go to idle and let user configure
        if (!spotify.hasCredentials()) {
            DEBUG_PRINTLN(F("Spotify credentials not configured"));
            spotifyConnected = false;
            initStarted = false;
            changeState(AppState::IDLE);
        }
        // Otherwise keep trying until timeout
    }
}

void handleIdleState() {
    static bool idleEntered = false;

    if (!idleEntered) {
        idleEntered = true;
        if (spotifyConnected) {
            leds.showIdle();
        } else {
            leds.showSpotifyError();
        }
        DEBUG_PRINTLN(F("Ready to scan NFC tags."));
    }

    // Check for NFC cards (interrupt-driven or polling)
    unsigned long now = millis();
    if (now - lastNfcCheck >= NFC_DEBOUNCE_TIME) {
        if (nfcReader.isNewCardPresent()) {
            lastNfcCheck = now;
            idleEntered = false;
            changeState(AppState::NFC_DETECTED);
        }
    }
}

void handleNfcDetected() {
    DEBUG_PRINTLN(F("NFC card detected"));
    leds.showNfcReading();
    webServer.notifyNfcTagDetected("reading...");
    changeState(AppState::NFC_READING);
}

void handleNfcReading() {
    NfcReadResult result = nfcReader.readSpotifyUri();

    if (result.success) {
        DEBUG_PRINT(F("Tag read successfully: "));
        DEBUG_PRINTLN(result.spotifyUri);
        currentNfcUri = result.spotifyUri;
        webServer.notifyNfcTagDetected(currentNfcUri);
        changeState(AppState::NFC_PROCESSING);
    } else {
        DEBUG_PRINT(F("NFC read failed: "));
        DEBUG_PRINTLN(result.errorMessage);
        webServer.notifyError(result.errorMessage);
        changeState(AppState::PLAYBACK_FAILED);
    }
}

void handleNfcProcessing() {
    leds.showTagProcessing();
    DEBUG_PRINTLN(F("Processing tag and sending to Spotify..."));

    // Check if we have a device selected
    if (!spotify.isDeviceAvailable()) {
        DEBUG_PRINTLN(F("No device selected. Use web interface to select a device."));
        webServer.notifyError("No device selected");
        changeState(AppState::PLAYBACK_FAILED);
        return;
    }

    // Check authentication
    if (!spotify.isAuthenticated()) {
        DEBUG_PRINTLN(F("Not authenticated with Spotify, refreshing token..."));
        if (!spotify.refreshToken()) {
            DEBUG_PRINTLN(F("Token refresh failed"));
            webServer.notifyError("Authentication failed");
            changeState(AppState::PLAYBACK_FAILED);
            return;
        }
    }

    // Attempt playback (uses exponential backoff internally)
    if (spotify.playUri(currentNfcUri)) {
        DEBUG_PRINTLN(F("Playback started successfully!"));
        webServer.notifyPlaybackStarted(currentNfcUri);
        changeState(AppState::PLAYBACK_SUCCESS);
    } else {
        DEBUG_PRINTLN(F("Failed to start playback"));
        webServer.notifyError("Playback failed");
        changeState(AppState::PLAYBACK_FAILED);
    }
}

void handlePlaybackSuccess() {
    static bool feedbackStarted = false;

    if (!feedbackStarted) {
        feedbackStarted = true;
        leds.showTagSuccess();
    }

    // Show success feedback for a duration, then return to idle
    if (millis() - stateEntryTime >= LED_FEEDBACK_DURATION) {
        feedbackStarted = false;
        currentNfcUri = "";
        changeState(AppState::IDLE);
    }
}

void handlePlaybackFailed() {
    static bool feedbackStarted = false;

    if (!feedbackStarted) {
        feedbackStarted = true;
        leds.showTagFailure();
    }

    // Show failure feedback for a duration, then return to idle
    if (millis() - stateEntryTime >= LED_FEEDBACK_DURATION) {
        feedbackStarted = false;
        currentNfcUri = "";
        changeState(AppState::IDLE);
    }
}

void handleErrorRecovery() {
    // Show error state briefly then restart
    if (millis() - stateEntryTime >= 3000) {
        DEBUG_PRINTLN(F("Restarting after error..."));
        ESP.restart();
    }
}

void handleWifiReconnecting() {
    static bool reconnectStarted = false;
    static unsigned long reconnectStartTime = 0;

    if (!reconnectStarted) {
        reconnectStarted = true;
        reconnectStartTime = millis();
        leds.showWifiError();
        DEBUG_PRINTLN(F("WiFi disconnected, attempting reconnection..."));
        WiFi.reconnect();
    }

    // Check if reconnected
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN(F("WiFi reconnected!"));
        DEBUG_PRINT(F("IP: "));
        DEBUG_PRINTLN(WiFi.localIP());
        wifiConnected = true;
        reconnectStarted = false;
        changeState(AppState::IDLE);
        webServer.notifyStatusChange();
        return;
    }

    // Timeout after 30 seconds
    if (millis() - reconnectStartTime > 30000) {
        DEBUG_PRINTLN(F("WiFi reconnection timeout"));
        reconnectStarted = false;
        changeState(AppState::ERROR_RECOVERY);
    }
}

// =============================================================================
// Helper Functions
// =============================================================================

void checkWifiStatus() {
    if (WiFi.status() != WL_CONNECTED && wifiConnected) {
        wifiConnected = false;
        changeState(AppState::WIFI_RECONNECTING);
    }
}

void updateSoundReactive() {
    if (leds.getState() == LedState::IDLE) {
        int audioLevel = analogRead(MIC_PIN);
        leds.updateSoundReactive(audioLevel);
    }
}

const char* stateToString(AppState state) {
    switch (state) {
        case AppState::BOOT: return "BOOT";
        case AppState::WIFI_CONNECTING: return "WIFI_CONNECTING";
        case AppState::WIFI_CONFIG_PORTAL: return "WIFI_CONFIG_PORTAL";
        case AppState::WIFI_CONNECTED: return "WIFI_CONNECTED";
        case AppState::SPOTIFY_INITIALIZING: return "SPOTIFY_INIT";
        case AppState::IDLE: return "IDLE";
        case AppState::NFC_DETECTED: return "NFC_DETECTED";
        case AppState::NFC_READING: return "NFC_READING";
        case AppState::NFC_PROCESSING: return "NFC_PROCESSING";
        case AppState::PLAYBACK_SUCCESS: return "PLAYBACK_SUCCESS";
        case AppState::PLAYBACK_FAILED: return "PLAYBACK_FAILED";
        case AppState::ERROR_RECOVERY: return "ERROR_RECOVERY";
        case AppState::WIFI_RECONNECTING: return "WIFI_RECONNECTING";
        default: return "UNKNOWN";
    }
}
