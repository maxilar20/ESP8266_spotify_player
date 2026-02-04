/**
 * @file SpotifyClient.h
 * @brief Spotify Web API Client for ESP8266
 *
 * Provides interface for Spotify playback control via the Web API.
 * Features:
 * - ArduinoJson for efficient JSON parsing
 * - Exponential backoff for retry logic
 * - Non-blocking operation support
 */

#ifndef SPOTIFY_CLIENT_H
#define SPOTIFY_CLIENT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "Config.h"

/**
 * @brief HTTP response container
 */
struct HttpResult
{
    int httpCode;
    String payload;

    bool isSuccess() const
    {
        return httpCode >= 200 && httpCode < 300;
    }

    bool isUnauthorized() const
    {
        return httpCode == 401;
    }

    bool isNotFound() const
    {
        return httpCode == 404;
    }

    bool isRateLimited() const
    {
        return httpCode == 429;
    }

    bool isServerError() const
    {
        return httpCode >= 500 && httpCode < 600;
    }

    bool shouldRetry() const
    {
        return isRateLimited() || isServerError() || httpCode == -1;
    }
};

/**
 * @brief Spotify device information
 */
struct SpotifyDevice
{
    String id;
    String name;
    String type;
    bool isActive;
    bool isRestricted;
};

/**
 * @brief HTTP methods supported by the client
 */
enum class HttpMethod
{
    GET,
    POST,
    PUT
};

/**
 * @brief Retry configuration for exponential backoff
 */
struct RetryConfig
{
    uint8_t maxRetries = SPOTIFY_MAX_RETRIES;
    uint32_t initialDelayMs = SPOTIFY_INITIAL_RETRY_DELAY;
    uint32_t maxDelayMs = SPOTIFY_MAX_RETRY_DELAY;
    float backoffMultiplier = SPOTIFY_BACKOFF_MULTIPLIER;
};

/**
 * @class SpotifyClient
 * @brief Client for Spotify Web API with ArduinoJson and exponential backoff
 */
class SpotifyClient
{
public:
    /**
     * @brief Construct a new Spotify Client with credentials
     * @param clientId Spotify application client ID
     * @param clientSecret Spotify application client secret
     * @param deviceName Target Spotify Connect device name
     * @param refreshToken OAuth refresh token
     */
    SpotifyClient(
        const String &clientId,
        const String &clientSecret,
        const String &deviceName,
        const String &refreshToken);

    /**
     * @brief Construct an empty Spotify Client (configure later)
     */
    SpotifyClient();

    /**
     * @brief Update credentials dynamically
     * @param clientId Spotify application client ID
     * @param clientSecret Spotify application client secret
     * @param deviceName Target Spotify Connect device name
     * @param refreshToken OAuth refresh token
     */
    void setCredentials(
        const String &clientId,
        const String &clientSecret,
        const String &deviceName,
        const String &refreshToken);

    /**
     * @brief Set retry configuration for exponential backoff
     * @param config Retry configuration settings
     */
    void setRetryConfig(const RetryConfig& config);

    /**
     * @brief Check if credentials are configured
     * @return true if all credentials are set
     */
    bool hasCredentials() const;

    /**
     * @brief Initialize the client (must be called after WiFi is connected)
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Fetch a new access token using the refresh token
     * @return true if token fetch successful
     */
    bool fetchAccessToken();

    /**
     * @brief Get available Spotify devices and find target device
     * @return true if target device found
     */
    bool discoverDevice();

    /**
     * @brief Play a Spotify URI (album, playlist, etc.)
     * @param contextUri Spotify URI to play
     * @return true if playback started successfully
     */
    bool playUri(const String &contextUri);

    /**
     * @brief Skip to next track
     * @return true if successful
     */
    bool nextTrack();

    /**
     * @brief Enable shuffle mode
     * @return true if successful
     */
    bool enableShuffle();

    /**
     * @brief Check if client is authenticated
     * @return true if access token is valid
     */
    bool isAuthenticated() const;

    /**
     * @brief Check if target device is available
     * @return true if device ID is set
     */
    bool isDeviceAvailable() const;

    /**
     * @brief Get list of available Spotify devices
     * @param devices Array to fill with device info (caller provides array)
     * @param maxDevices Maximum number of devices to return
     * @return Number of devices found
     */
    int getAvailableDevices(SpotifyDevice* devices, int maxDevices);

    /**
     * @brief Get devices as JSON string (for web API)
     * @return JSON array of devices
     */
    String getDevicesJson();

    /**
     * @brief Set the target device by ID
     * @param deviceId Spotify device ID to use
     * @return true if device was set successfully
     */
    bool setDeviceById(const String& deviceId);

    /**
     * @brief Set the target device by name
     * @param name Spotify device name to use
     * @return true if device was found and set
     */
    bool setDeviceByName(const String& name);

    /**
     * @brief Get current device ID
     * @return Current device ID or empty string
     */
    String getDeviceId() const;

    /**
     * @brief Get current device name
     * @return Current device name or empty string
     */
    String getDeviceName() const;

    /**
     * @brief Refresh the access token
     * @return true if successful
     */
    bool refreshToken();

private:
    WiFiClientSecure wifiClient_;

    String clientId_;
    String clientSecret_;
    String refreshToken_;
    String accessToken_;
    String deviceName_;
    String deviceId_;

    RetryConfig retryConfig_;
    uint32_t lastRetryTime_ = 0;
    uint8_t currentRetryCount_ = 0;

    /**
     * @brief Make an authenticated API call with retry logic
     * @param method HTTP method
     * @param url Full URL to call
     * @param body Request body (optional)
     * @return HttpResult with response code and payload
     */
    HttpResult callApi(HttpMethod method, const String &url, const String &body = "");

    /**
     * @brief Make an authenticated API call with exponential backoff
     * @param method HTTP method
     * @param url Full URL to call
     * @param body Request body (optional)
     * @return HttpResult with response code and payload
     */
    HttpResult callApiWithRetry(HttpMethod method, const String &url, const String &body = "");

    /**
     * @brief Calculate delay for exponential backoff
     * @param retryCount Current retry attempt number
     * @return Delay in milliseconds
     */
    uint32_t calculateBackoffDelay(uint8_t retryCount) const;

    /**
     * @brief Parse devices from JSON response using ArduinoJson
     * @param json JSON response from devices endpoint
     * @param devices Array to fill with device info
     * @param maxDevices Maximum number of devices
     * @return Number of devices parsed
     */
    int parseDevicesJson(const String& json, SpotifyDevice* devices, int maxDevices);

    /**
     * @brief Extract device ID from devices JSON response
     * @param json JSON response from devices endpoint
     * @return Device ID or empty string if not found
     */
    String extractDeviceId(const String &json);

    /**
     * @brief Build authorization header value
     * @return Base64 encoded client credentials
     */
    String buildBasicAuthHeader() const;

    /**
     * @brief Build Bearer token header value
     * @return Bearer token string
     */
    String buildBearerAuthHeader() const;
};

#endif // SPOTIFY_CLIENT_H
