/**
 * @file ConfigManager.cpp
 * @brief Implementation of web-based configuration manager
 */

#include "ConfigManager.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>

// Spotify OAuth endpoints (local constants - different from Config.h macros)
static const char* SPOTIFY_AUTHORIZE_ENDPOINT = "https://accounts.spotify.com/authorize";
static const char* SPOTIFY_TOKEN_ENDPOINT = "https://accounts.spotify.com/api/token";

// OAuth scopes needed for playback control
static const char* SPOTIFY_SCOPES = "user-read-playback-state user-modify-playback-state user-read-currently-playing";

ConfigManager::ConfigManager()
    : server_(nullptr)
    , serverRunning_(false)
{
}

bool ConfigManager::begin() {
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        DEBUG_PRINTLN(F("Failed to mount LittleFS"));
        // Try formatting
        if (LittleFS.format()) {
            DEBUG_PRINTLN(F("LittleFS formatted, retrying mount..."));
            if (!LittleFS.begin()) {
                DEBUG_PRINTLN(F("LittleFS mount failed after format"));
                return false;
            }
        } else {
            return false;
        }
    }

    DEBUG_PRINTLN(F("LittleFS mounted successfully"));

    // Load existing configuration
    loadConfig();

    return true;
}

void ConfigManager::startWebServer(uint16_t port) {
    if (serverRunning_) {
        return;
    }

    // Build redirect URI based on current IP
    redirectUri_ = "http://" + WiFi.localIP().toString() + "/callback";

    server_ = new ESP8266WebServer(port);

    // Setup routes
    server_->on("/", HTTP_GET, std::bind(&ConfigManager::handleRoot, this));
    server_->on("/save", HTTP_POST, std::bind(&ConfigManager::handleSave, this));
    server_->on("/callback", HTTP_GET, std::bind(&ConfigManager::handleCallback, this));
    server_->on("/status", HTTP_GET, std::bind(&ConfigManager::handleStatus, this));
    server_->on("/clear", HTTP_POST, std::bind(&ConfigManager::handleClear, this));
    server_->onNotFound(std::bind(&ConfigManager::handleNotFound, this));

    server_->begin();
    serverRunning_ = true;

    DEBUG_PRINT(F("Config server started at http://"));
    DEBUG_PRINT(WiFi.localIP());
    DEBUG_PRINT(F(":"));
    DEBUG_PRINTLN(port);
}

void ConfigManager::stopWebServer() {
    if (server_ != nullptr) {
        server_->stop();
        delete server_;
        server_ = nullptr;
    }
    serverRunning_ = false;
}

void ConfigManager::handleClient() {
    if (serverRunning_ && server_ != nullptr) {
        server_->handleClient();
    }
}

bool ConfigManager::isConfigured() const {
    return config_.isValid();
}

SpotifyConfig ConfigManager::getConfig() const {
    return config_;
}

bool ConfigManager::saveConfig(const SpotifyConfig& config) {
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        DEBUG_PRINTLN(F("Failed to open config file for writing"));
        return false;
    }

    // Simple JSON serialization
    String json = "{";
    json += "\"clientId\":\"" + config.clientId + "\",";
    json += "\"clientSecret\":\"" + config.clientSecret + "\",";
    json += "\"refreshToken\":\"" + config.refreshToken + "\",";
    json += "\"deviceName\":\"" + config.deviceName + "\"";
    json += "}";

    file.print(json);
    file.close();

    config_ = config;

    DEBUG_PRINTLN(F("Configuration saved"));
    return true;
}

bool ConfigManager::loadConfig() {
    if (!LittleFS.exists(CONFIG_FILE)) {
        DEBUG_PRINTLN(F("No config file found"));
        return false;
    }

    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        DEBUG_PRINTLN(F("Failed to open config file"));
        return false;
    }

    String json = file.readString();
    file.close();

    config_.clientId = parseJsonValue("clientId", json);
    config_.clientSecret = parseJsonValue("clientSecret", json);
    config_.refreshToken = parseJsonValue("refreshToken", json);
    config_.deviceName = parseJsonValue("deviceName", json);

    DEBUG_PRINTLN(F("Configuration loaded"));
    DEBUG_PRINT(F("Device name: "));
    DEBUG_PRINTLN(config_.deviceName);

    return config_.isValid();
}

bool ConfigManager::clearConfig() {
    if (LittleFS.exists(CONFIG_FILE)) {
        LittleFS.remove(CONFIG_FILE);
    }

    config_ = SpotifyConfig();
    DEBUG_PRINTLN(F("Configuration cleared"));
    return true;
}

String ConfigManager::getAuthorizationUrl() const {
    String url = SPOTIFY_AUTHORIZE_ENDPOINT;
    url += "?client_id=" + urlEncode(config_.clientId);
    url += "&response_type=code";
    url += "&redirect_uri=" + urlEncode(redirectUri_);
    url += "&scope=" + urlEncode(SPOTIFY_SCOPES);
    url += "&show_dialog=true";

    return url;
}

// =============================================================================
// Web Server Route Handlers
// =============================================================================

void ConfigManager::handleRoot() {
    server_->send(200, "text/html", generateConfigPage());
}

void ConfigManager::handleSave() {
    SpotifyConfig newConfig;
    newConfig.clientId = server_->arg("client_id");
    newConfig.clientSecret = server_->arg("client_secret");
    newConfig.deviceName = server_->arg("device_name");

    // Keep existing refresh token if we have one and none was provided
    if (server_->hasArg("refresh_token") && server_->arg("refresh_token").length() > 0) {
        newConfig.refreshToken = server_->arg("refresh_token");
    } else {
        newConfig.refreshToken = config_.refreshToken;
    }

    if (newConfig.clientId.isEmpty() || newConfig.clientSecret.isEmpty() || newConfig.deviceName.isEmpty()) {
        server_->send(200, "text/html", generateErrorPage("Client ID, Client Secret, and Device Name are required"));
        return;
    }

    // Save the config (even without refresh token - user will authorize next)
    config_ = newConfig;
    saveConfig(config_);

    // If no refresh token, redirect to Spotify authorization
    if (config_.refreshToken.isEmpty()) {
        String authUrl = getAuthorizationUrl();
        server_->sendHeader("Location", authUrl, true);
        server_->send(302, "text/plain", "Redirecting to Spotify...");
    } else {
        server_->send(200, "text/html", generateSuccessPage());
    }
}

void ConfigManager::handleCallback() {
    // Handle OAuth callback from Spotify
    if (server_->hasArg("error")) {
        String error = server_->arg("error");
        DEBUG_PRINT(F("OAuth error: "));
        DEBUG_PRINTLN(error);
        server_->send(200, "text/html", generateErrorPage("Spotify authorization failed: " + error));
        return;
    }

    if (!server_->hasArg("code")) {
        server_->send(200, "text/html", generateErrorPage("No authorization code received"));
        return;
    }

    String code = server_->arg("code");
    DEBUG_PRINTLN(F("Received authorization code, exchanging for token..."));

    if (exchangeCodeForToken(code)) {
        server_->send(200, "text/html", generateSuccessPage());
    } else {
        server_->send(200, "text/html", generateErrorPage("Failed to exchange code for token"));
    }
}

void ConfigManager::handleStatus() {
    server_->send(200, "text/html", generateStatusPage());
}

void ConfigManager::handleClear() {
    clearConfig();
    server_->sendHeader("Location", "/", true);
    server_->send(302, "text/plain", "Configuration cleared");
}

void ConfigManager::handleNotFound() {
    server_->send(404, "text/plain", "Not Found");
}

// =============================================================================
// Token Exchange
// =============================================================================

bool ConfigManager::exchangeCodeForToken(const String& code) {
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation

    HTTPClient http;

    String body = "grant_type=authorization_code";
    body += "&code=" + urlEncode(code);
    body += "&redirect_uri=" + urlEncode(redirectUri_);

    String credentials = config_.clientId + ":" + config_.clientSecret;
    String authorization = base64::encode(credentials);

    http.begin(client, SPOTIFY_TOKEN_ENDPOINT);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Basic " + authorization);

    int httpCode = http.POST(body);

    if (httpCode != 200) {
        DEBUG_PRINT(F("Token exchange failed: "));
        DEBUG_PRINTLN(httpCode);
        if (httpCode > 0) {
            DEBUG_PRINTLN(http.getString());
        }
        http.end();
        return false;
    }

    String response = http.getString();
    http.end();

    // Extract refresh token
    String refreshToken = parseJsonValue("refresh_token", response);

    if (refreshToken.isEmpty()) {
        DEBUG_PRINTLN(F("No refresh token in response"));
        return false;
    }

    config_.refreshToken = refreshToken;
    saveConfig(config_);

    DEBUG_PRINTLN(F("Successfully obtained and saved refresh token"));
    return true;
}

// =============================================================================
// HTML Page Generators
// =============================================================================

String ConfigManager::generateConfigPage() {
    String html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Spotify Player Configuration</title>
    <style>
        * { box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1DB954 0%, #191414 100%);
            min-height: 100vh;
            margin: 0;
            padding: 20px;
            color: #fff;
        }
        .container {
            max-width: 500px;
            margin: 0 auto;
            background: rgba(0,0,0,0.7);
            border-radius: 12px;
            padding: 30px;
        }
        h1 {
            text-align: center;
            margin-bottom: 10px;
        }
        .subtitle {
            text-align: center;
            color: #b3b3b3;
            margin-bottom: 30px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #1DB954;
            font-weight: 500;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 12px;
            border: 1px solid #333;
            border-radius: 6px;
            background: #282828;
            color: #fff;
            font-size: 14px;
        }
        input:focus {
            outline: none;
            border-color: #1DB954;
        }
        .help-text {
            font-size: 12px;
            color: #888;
            margin-top: 5px;
        }
        button {
            width: 100%;
            padding: 14px;
            background: #1DB954;
            color: #000;
            border: none;
            border-radius: 30px;
            font-size: 16px;
            font-weight: 700;
            cursor: pointer;
            margin-top: 10px;
        }
        button:hover {
            background: #1ed760;
        }
        .status {
            background: #282828;
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
        }
        .status-item {
            display: flex;
            justify-content: space-between;
            padding: 5px 0;
        }
        .status-ok { color: #1DB954; }
        .status-missing { color: #ff6b6b; }
        .instructions {
            background: #282828;
            padding: 15px;
            border-radius: 6px;
            margin-top: 20px;
            font-size: 13px;
        }
        .instructions h3 {
            margin-top: 0;
            color: #1DB954;
        }
        .instructions ol {
            padding-left: 20px;
            margin: 0;
        }
        .instructions li {
            margin-bottom: 8px;
        }
        a { color: #1DB954; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎵 Spotify Player</h1>
        <p class="subtitle">ESP8266 NFC Controller Setup</p>

        <div class="status">
            <div class="status-item">
                <span>Client ID:</span>
                <span class=")";

    html += config_.clientId.isEmpty() ? "status-missing\">Not set" : "status-ok\">Set";
    html += R"(</span>
            </div>
            <div class="status-item">
                <span>Client Secret:</span>
                <span class=")";

    html += config_.clientSecret.isEmpty() ? "status-missing\">Not set" : "status-ok\">Set";
    html += R"(</span>
            </div>
            <div class="status-item">
                <span>Device Name:</span>
                <span class=")";

    html += config_.deviceName.isEmpty() ? "status-missing\">Not set" : "status-ok\">" + config_.deviceName;
    html += R"(</span>
            </div>
            <div class="status-item">
                <span>Spotify Authorized:</span>
                <span class=")";

    html += config_.refreshToken.isEmpty() ? "status-missing\">No" : "status-ok\">Yes";
    html += R"(</span>
            </div>
        </div>

        <form action="/save" method="POST">
            <div class="form-group">
                <label>Client ID</label>
                <input type="text" name="client_id" value=")";
    html += config_.clientId;
    html += R"(" placeholder="Enter your Spotify Client ID">
                <div class="help-text">From Spotify Developer Dashboard</div>
            </div>

            <div class="form-group">
                <label>Client Secret</label>
                <input type="password" name="client_secret" value=")";
    html += config_.clientSecret;
    html += R"(" placeholder="Enter your Spotify Client Secret">
                <div class="help-text">From Spotify Developer Dashboard</div>
            </div>

            <div class="form-group">
                <label>Spotify Device Name</label>
                <input type="text" name="device_name" value=")";
    html += config_.deviceName;
    html += R"(" placeholder="e.g., Living Room Speaker">
                <div class="help-text">Exact name of the Spotify Connect device to control</div>
            </div>

            <button type="submit">)";

    html += config_.refreshToken.isEmpty() ? "Save &amp; Authorize with Spotify" : "Save Configuration";
    html += R"(</button>
        </form>

        <div class="instructions">
            <h3>📋 Setup Instructions</h3>
            <ol>
                <li>Go to <a href="https://developer.spotify.com/dashboard" target="_blank">Spotify Developer Dashboard</a></li>
                <li>Create a new app (any name)</li>
                <li>In app settings, add this Redirect URI:<br><code>http://)";
    html += WiFi.localIP().toString();
    html += R"(/callback</code></li>
                <li>Copy your Client ID and Client Secret here</li>
                <li>Enter the exact name of your Spotify device</li>
                <li>Click Save to authorize with Spotify</li>
            </ol>
        </div>
    </div>
</body>
</html>)";

    return html;
}

String ConfigManager::generateSuccessPage() {
    String html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuration Saved</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1DB954 0%, #191414 100%);
            min-height: 100vh;
            margin: 0;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #fff;
        }
        .container {
            text-align: center;
            background: rgba(0,0,0,0.7);
            border-radius: 12px;
            padding: 40px;
            max-width: 400px;
        }
        .checkmark {
            font-size: 64px;
            margin-bottom: 20px;
        }
        h1 { margin-bottom: 10px; }
        p { color: #b3b3b3; }
        a {
            display: inline-block;
            margin-top: 20px;
            padding: 12px 30px;
            background: #1DB954;
            color: #000;
            text-decoration: none;
            border-radius: 30px;
            font-weight: 700;
        }
        .note {
            margin-top: 20px;
            padding: 15px;
            background: #282828;
            border-radius: 6px;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="checkmark">✅</div>
        <h1>Configuration Saved!</h1>
        <p>Your Spotify credentials have been saved successfully.</p>
        <div class="note">
            The device will now use these settings. If playback doesn't work,
            make sure your Spotify device is online and playing.
        </div>
        <a href="/">Back to Settings</a>
    </div>
</body>
</html>)";

    return html;
}

String ConfigManager::generateErrorPage(const String& error) {
    String html = R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Error</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #ff6b6b 0%, #191414 100%);
            min-height: 100vh;
            margin: 0;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #fff;
        }
        .container {
            text-align: center;
            background: rgba(0,0,0,0.7);
            border-radius: 12px;
            padding: 40px;
            max-width: 400px;
        }
        .icon { font-size: 64px; margin-bottom: 20px; }
        h1 { margin-bottom: 10px; }
        p { color: #ff6b6b; }
        a {
            display: inline-block;
            margin-top: 20px;
            padding: 12px 30px;
            background: #1DB954;
            color: #000;
            text-decoration: none;
            border-radius: 30px;
            font-weight: 700;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="icon">❌</div>
        <h1>Configuration Error</h1>
        <p>)";
    html += error;
    html += R"(</p>
        <a href="/">Try Again</a>
    </div>
</body>
</html>)";

    return html;
}

String ConfigManager::generateStatusPage() {
    String html = R"({"configured":)";
    html += isConfigured() ? "true" : "false";
    html += R"(,"clientId":")";
    html += config_.clientId.isEmpty() ? "" : "***";
    html += R"(","hasRefreshToken":)";
    html += config_.refreshToken.isEmpty() ? "false" : "true";
    html += R"(,"deviceName":")";
    html += config_.deviceName;
    html += R"("})";

    server_->send(200, "application/json", html);
    return "";
}

// =============================================================================
// Utility Functions
// =============================================================================

String ConfigManager::urlEncode(const String& str) const {
    String encoded = "";
    char c;
    char code0;
    char code1;

    for (unsigned int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == ' ') {
            encoded += '+';
        } else if (isalnum(c)) {
            encoded += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) {
                code0 = c - 10 + 'A';
            }
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }

    return encoded;
}

String ConfigManager::parseJsonValue(const String& key, const String& json) {
    String value;
    String searchKey = "\"" + key + "\"";
    int index = json.indexOf(searchKey);

    if (index < 0) {
        return value;
    }

    // Find the colon after the key
    int colonIndex = json.indexOf(':', index + searchKey.length());
    if (colonIndex < 0) {
        return value;
    }

    // Find the opening quote of the value
    int startQuote = json.indexOf('"', colonIndex);
    if (startQuote < 0) {
        return value;
    }

    // Find the closing quote
    int endQuote = json.indexOf('"', startQuote + 1);
    if (endQuote < 0) {
        return value;
    }

    value = json.substring(startQuote + 1, endQuote);
    return value;
}
