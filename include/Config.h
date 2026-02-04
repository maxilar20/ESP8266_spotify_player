/**
 * @file Config.h
 * @brief Configuration settings for the ESP8266 Spotify Player
 *
 * This file contains all configurable parameters for the project.
 * Copy this file to Config_local.h and modify with your credentials.
 * Config_local.h is gitignored to protect sensitive information.
 */

#ifndef CONFIG_H
#define CONFIG_H

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

// =============================================================================
// Spotify API Configuration
// =============================================================================

// IMPORTANT: Replace these with your actual Spotify API credentials
// Get these from https://developer.spotify.com/dashboard

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
