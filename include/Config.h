/**
 * @file Config.h
 * @brief Configuration settings for the ESP8266 Spotify Player
 *
 * This file contains all configurable parameters for the project.
 * Copy Config_local.h.example to Config_local.h and modify with your credentials.
 * Config_local.h is gitignored to protect sensitive information.
 *
 * Features:
 * - Exponential backoff configuration
 * - NFC interrupt support
 * - Async operation settings
 */

#ifndef CONFIG_H
#define CONFIG_H

// Include local configuration overrides if they exist
#ifdef __has_include
  #if __has_include("Config_local.h")
    #include "Config_local.h"
  #endif
#endif

// =============================================================================
// Hardware Configuration
// =============================================================================

// NeoPixel LED Strip Settings
#define LED_PIN 4         // GPIO pin connected to NeoPixel data line
#define NUM_LEDS 8        // Number of LEDs in the strip
#define LED_BRIGHTNESS 50 // Default brightness (0-255)
#define LED_TYPE NEO_GRB + NEO_KHZ800

// MFRC522 NFC Reader Settings
#define NFC_RST_PIN 0 // GPIO pin for NFC reader reset
#define NFC_SS_PIN 15 // GPIO pin for NFC reader SPI slave select

// NFC Interrupt Pin (optional - set to -1 for polling mode)
// Connect MFRC522 IRQ pin to this GPIO for interrupt-driven detection
#ifndef NFC_IRQ_PIN
#define NFC_IRQ_PIN -1  // -1 = polling mode, or use GPIO pin (e.g., 2)
#endif

// Microphone/Audio Input Settings
#define MIC_PIN A0             // Analog pin for microphone input
#define MIC_SAMPLE_INTERVAL 35 // Milliseconds between audio samples

// =============================================================================
// Network Configuration
// =============================================================================

// WiFi Manager Settings
#define WIFI_AP_NAME "SpotifyPlayer-AP"
#define WIFI_AP_PASSWORD "configme123"
#define WIFI_RECONNECT_DELAY 20000 // Milliseconds between reconnection attempts

// Web Server Settings
#define WEB_SERVER_PORT 80

// =============================================================================
// Spotify API Configuration
// =============================================================================

// Get these from https://developer.spotify.com/dashboard
// See docs/SPOTIFY_SETUP.md for how to get the refresh token

#ifndef SPOTIFY_CLIENT_ID
#define SPOTIFY_CLIENT_ID "your_client_id_here"
#endif

#ifndef SPOTIFY_CLIENT_SECRET
#define SPOTIFY_CLIENT_SECRET "your_client_secret_here"
#endif

#ifndef SPOTIFY_REFRESH_TOKEN
#define SPOTIFY_REFRESH_TOKEN "your_refresh_token_here"
#endif

#ifndef SPOTIFY_DEVICE_NAME
#define SPOTIFY_DEVICE_NAME "Your Spotify Device Name"
#endif

// Spotify API Endpoints
#define SPOTIFY_TOKEN_URL "https://accounts.spotify.com/api/token"
#define SPOTIFY_API_BASE_URL "https://api.spotify.com/v1"
#define SPOTIFY_DEVICES_URL SPOTIFY_API_BASE_URL "/me/player/devices"
#define SPOTIFY_PLAY_URL SPOTIFY_API_BASE_URL "/me/player/play"
#define SPOTIFY_NEXT_URL SPOTIFY_API_BASE_URL "/me/player/next"
#define SPOTIFY_SHUFFLE_URL SPOTIFY_API_BASE_URL "/me/player/shuffle"

// =============================================================================
// Exponential Backoff Configuration
// =============================================================================

// Retry settings for Spotify API calls
#ifndef SPOTIFY_MAX_RETRIES
#define SPOTIFY_MAX_RETRIES 3
#endif

#ifndef SPOTIFY_INITIAL_RETRY_DELAY
#define SPOTIFY_INITIAL_RETRY_DELAY 1000  // Initial delay in ms
#endif

#ifndef SPOTIFY_MAX_RETRY_DELAY
#define SPOTIFY_MAX_RETRY_DELAY 10000     // Maximum delay in ms
#endif

#ifndef SPOTIFY_BACKOFF_MULTIPLIER
#define SPOTIFY_BACKOFF_MULTIPLIER 2.0f   // Multiplier for exponential backoff
#endif

// =============================================================================
// State Machine Timing Configuration
// =============================================================================

// Non-blocking timing intervals (in milliseconds)
#define STATE_MACHINE_INTERVAL 10         // Main loop state machine interval
#define LED_FEEDBACK_DURATION 2000        // Duration to show success/failure LEDs
#define SPOTIFY_INIT_TIMEOUT 30000        // Timeout for Spotify initialization
#define NFC_DEBOUNCE_TIME 500             // Debounce time for NFC reads

// =============================================================================
// Debug Configuration
// =============================================================================

// Uncomment to enable debug output
#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#endif // CONFIG_H
