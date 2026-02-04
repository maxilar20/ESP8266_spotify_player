/**
 * @file WebServer.h
 * @brief Web Server for device selection and configuration
 *
 * Hosts a simple web interface for selecting Spotify devices.
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "SpotifyClient.h"
#include "LedController.h"
#include "Config.h"

/**
 * @class WebServerController
 * @brief Manages web interface for device selection
 */
class WebServerController
{
public:
    /**
     * @brief Construct web server controller
     * @param port Port to listen on
     * @param spotifyClient Reference to Spotify client
     * @param ledController Reference to LED controller for feedback
     */
    WebServerController(uint16_t port, SpotifyClient& spotifyClient, LedController& ledController);

    /**
     * @brief Initialize and start the web server
     */
    void begin();

    /**
     * @brief Handle incoming client requests (call in main loop)
     */
    void handleClient();

    /**
     * @brief Get the server's IP address as string
     * @return IP address string
     */
    String getIPAddress() const;

    /**
     * @brief Notify of status change (no-op for sync server, kept for API compatibility)
     */
    void notifyStatusChange();

    /**
     * @brief Notify of NFC tag detection (no-op for sync server, kept for API compatibility)
     */
    void notifyNfcTagDetected(const String& uri);

    /**
     * @brief Notify of playback started (no-op for sync server, kept for API compatibility)
     */
    void notifyPlaybackStarted(const String& uri);

    /**
     * @brief Notify of error (no-op for sync server, kept for API compatibility)
     */
    void notifyError(const String& errorMessage);

private:
    ESP8266WebServer server_;
    SpotifyClient& spotify_;
    LedController& leds_;

    /**
     * @brief Handle root page request
     */
    void handleRoot();

    /**
     * @brief Handle API request for device list
     */
    void handleGetDevices();

    /**
     * @brief Handle API request to set device
     */
    void handleSetDevice();

    /**
     * @brief Handle API request for current status
     */
    void handleStatus();

    /**
     * @brief Handle API request for WiFi info
     */
    void handleWifiInfo();

    /**
     * @brief Handle API request to reset WiFi settings
     */
    void handleWifiReset();

    /**
     * @brief Handle API request to restart device
     */
    void handleRestart();

    /**
     * @brief Handle 404 not found
     */
    void handleNotFound();
};

#endif // WEB_SERVER_H
