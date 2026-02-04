/**
 * @file ConfigManager.h
 * @brief Web-based configuration manager for Spotify credentials
 *
 * Provides a web interface to configure Spotify API credentials
 * and complete OAuth authorization without hardcoding.
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "Config.h"

// Configuration file path
#define CONFIG_FILE "/spotify_config.json"

/**
 * @brief Spotify configuration data structure
 */
struct SpotifyConfig {
    String clientId;
    String clientSecret;
    String refreshToken;
    String deviceName;

    bool isValid() const {
        return !clientId.isEmpty() &&
               !clientSecret.isEmpty() &&
               !refreshToken.isEmpty() &&
               !deviceName.isEmpty();
    }
};

/**
 * @class ConfigManager
 * @brief Manages Spotify configuration via web interface
 */
class ConfigManager {
public:
    ConfigManager();

    /**
     * @brief Initialize the config manager
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Start the web server for configuration
     * @param port HTTP server port (default 80)
     */
    void startWebServer(uint16_t port = 80);

    /**
     * @brief Stop the web server
     */
    void stopWebServer();

    /**
     * @brief Handle web server requests (call in loop)
     */
    void handleClient();

    /**
     * @brief Check if configuration is complete
     * @return true if all required fields are set
     */
    bool isConfigured() const;

    /**
     * @brief Get the current configuration
     * @return SpotifyConfig structure
     */
    SpotifyConfig getConfig() const;

    /**
     * @brief Save configuration to flash
     * @param config Configuration to save
     * @return true if save successful
     */
    bool saveConfig(const SpotifyConfig& config);

    /**
     * @brief Load configuration from flash
     * @return true if load successful
     */
    bool loadConfig();

    /**
     * @brief Clear saved configuration
     * @return true if cleared successfully
     */
    bool clearConfig();

    /**
     * @brief Get the OAuth authorization URL
     * @return Full Spotify authorization URL
     */
    String getAuthorizationUrl() const;

    /**
     * @brief Check if web server is running
     * @return true if server is active
     */
    bool isServerRunning() const { return serverRunning_; }

private:
    ESP8266WebServer* server_;
    SpotifyConfig config_;
    bool serverRunning_;
    String redirectUri_;

    // Web server route handlers
    void handleRoot();
    void handleSave();
    void handleCallback();
    void handleStatus();
    void handleClear();
    void handleNotFound();

    // Token exchange
    bool exchangeCodeForToken(const String& code);

    // HTML page generators
    String generateConfigPage();
    String generateSuccessPage();
    String generateErrorPage(const String& error);
    String generateStatusPage();

    // Utility functions
    String urlEncode(const String& str) const;
    String parseJsonValue(const String& key, const String& json);
};

#endif // CONFIG_MANAGER_H
