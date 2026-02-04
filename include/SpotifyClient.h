/**
 * @file SpotifyClient.h
 * @brief Spotify Web API Client for ESP8266
 *
 * Provides interface for Spotify playback control via the Web API.
 */

#ifndef SPOTIFY_CLIENT_H
#define SPOTIFY_CLIENT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
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
 * @class SpotifyClient
 * @brief Client for Spotify Web API
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

private:
    WiFiClientSecure wifiClient_;

    String clientId_;
    String clientSecret_;
    String refreshToken_;
    String accessToken_;
    String deviceName_;
    String deviceId_;

    /**
     * @brief Make an authenticated API call
     * @param method HTTP method
     * @param url Full URL to call
     * @param body Request body (optional)
     * @return HttpResult with response code and payload
     */
    HttpResult callApi(HttpMethod method, const String &url, const String &body = "");

    /**
     * @brief Parse a JSON value by key (simple parser)
     * @param key Key to find
     * @param json JSON string to parse
     * @return Value string or empty if not found
     */
    static String parseJsonValue(const String &key, const String &json);

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
