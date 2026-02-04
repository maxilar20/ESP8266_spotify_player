/**
 * @file SpotifyClient.cpp
 * @brief Implementation of Spotify Web API Client
 *
 * Features:
 * - ArduinoJson for efficient JSON parsing
 * - Exponential backoff for retry logic
 * - Non-blocking friendly design
 */

#include "SpotifyClient.h"
#include <ESP8266HTTPClient.h>
#include <base64.h>

SpotifyClient::SpotifyClient()
    : clientId_()
    , clientSecret_()
    , refreshToken_()
    , deviceName_()
{
}

SpotifyClient::SpotifyClient(
    const String& clientId,
    const String& clientSecret,
    const String& deviceName,
    const String& refreshToken
)
    : clientId_(clientId)
    , clientSecret_(clientSecret)
    , refreshToken_(refreshToken)
    , deviceName_(deviceName)
{
}

void SpotifyClient::setCredentials(
    const String& clientId,
    const String& clientSecret,
    const String& deviceName,
    const String& refreshToken
) {
    clientId_ = clientId;
    clientSecret_ = clientSecret;
    deviceName_ = deviceName;
    refreshToken_ = refreshToken;
    // Clear existing tokens when credentials change
    accessToken_ = "";
    deviceId_ = "";
}

void SpotifyClient::setRetryConfig(const RetryConfig& config) {
    retryConfig_ = config;
}

bool SpotifyClient::hasCredentials() const {
    return !clientId_.isEmpty() &&
           !clientSecret_.isEmpty() &&
           !refreshToken_.isEmpty() &&
           !deviceName_.isEmpty();
}

bool SpotifyClient::begin() {
    // Use insecure mode for ESP8266 (no certificate validation)
    // This is acceptable for this use case but should be noted
    wifiClient_.setInsecure();

    if (!fetchAccessToken()) {
        DEBUG_PRINTLN(F("Failed to fetch initial access token"));
        return false;
    }

    if (!discoverDevice()) {
        DEBUG_PRINTLN(F("Failed to discover target device"));
        return false;
    }

    return true;
}

bool SpotifyClient::fetchAccessToken() {
    HTTPClient http;

    String body = "grant_type=refresh_token&refresh_token=" + refreshToken_;
    String authorization = buildBasicAuthHeader();

    http.begin(wifiClient_, SPOTIFY_TOKEN_URL);
    http.addHeader(F("Content-Type"), F("application/x-www-form-urlencoded"));
    http.addHeader(F("Authorization"), "Basic " + authorization);

    int httpCode = http.POST(body);

    if (httpCode != 200) {
        DEBUG_PRINT(F("Token fetch failed with code: "));
        DEBUG_PRINTLN(httpCode);
        if (httpCode > 0) {
            DEBUG_PRINTLN(http.getString());
        }
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    // Parse with ArduinoJson
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        DEBUG_PRINT(F("JSON parse error: "));
        DEBUG_PRINTLN(error.c_str());
        return false;
    }

    accessToken_ = doc["access_token"].as<String>();

    if (accessToken_.isEmpty()) {
        DEBUG_PRINTLN(F("Failed to parse access token from response"));
        return false;
    }

    DEBUG_PRINTLN(F("Successfully obtained access token"));
    return true;
}

bool SpotifyClient::discoverDevice() {
    HttpResult result = callApiWithRetry(HttpMethod::GET, SPOTIFY_DEVICES_URL);

    if (!result.isSuccess()) {
        DEBUG_PRINT(F("Failed to get devices: "));
        DEBUG_PRINTLN(result.httpCode);
        return false;
    }

    deviceId_ = extractDeviceId(result.payload);

    if (deviceId_.isEmpty()) {
        DEBUG_PRINT(F("Device '"));
        DEBUG_PRINT(deviceName_);
        DEBUG_PRINTLN(F("' not found"));
        return false;
    }

    DEBUG_PRINT(F("Found device ID: "));
    DEBUG_PRINTLN(deviceId_);
    return true;
}

bool SpotifyClient::playUri(const String& contextUri) {
    DEBUG_PRINT(F("Playing URI: "));
    DEBUG_PRINTLN(contextUri);

    // Build JSON body with ArduinoJson
    JsonDocument doc;
    doc["context_uri"] = contextUri;
    JsonObject offset = doc["offset"].to<JsonObject>();
    offset["position"] = 0;
    offset["position_ms"] = 0;

    String body;
    serializeJson(doc, body);

    String url = String(SPOTIFY_PLAY_URL) + "?device_id=" + deviceId_;

    HttpResult result = callApiWithRetry(HttpMethod::PUT, url, body);

    // Handle specific error cases
    if (result.isNotFound()) {
        DEBUG_PRINTLN(F("Device not found, rediscovering..."));
        if (discoverDevice()) {
            url = String(SPOTIFY_PLAY_URL) + "?device_id=" + deviceId_;
            result = callApiWithRetry(HttpMethod::PUT, url, body);
        }
    } else if (result.isUnauthorized()) {
        DEBUG_PRINTLN(F("Token expired, refreshing..."));
        if (fetchAccessToken()) {
            result = callApiWithRetry(HttpMethod::PUT, url, body);
        }
    }

    if (result.isSuccess()) {
        // Enable shuffle after starting playback
        enableShuffle();
        return true;
    }

    DEBUG_PRINT(F("Play failed with code: "));
    DEBUG_PRINTLN(result.httpCode);
    return false;
}

bool SpotifyClient::nextTrack() {
    DEBUG_PRINTLN(F("Skipping to next track"));

    String url = String(SPOTIFY_NEXT_URL) + "?device_id=" + deviceId_;
    HttpResult result = callApiWithRetry(HttpMethod::POST, url);

    return result.isSuccess();
}

bool SpotifyClient::enableShuffle() {
    DEBUG_PRINTLN(F("Enabling shuffle"));

    String url = String(SPOTIFY_SHUFFLE_URL) + "?state=true&device_id=" + deviceId_;
    HttpResult result = callApiWithRetry(HttpMethod::PUT, url);

    return result.isSuccess();
}

bool SpotifyClient::isAuthenticated() const {
    return !accessToken_.isEmpty();
}

bool SpotifyClient::isDeviceAvailable() const {
    return !deviceId_.isEmpty();
}

String SpotifyClient::getDeviceId() const {
    return deviceId_;
}

String SpotifyClient::getDeviceName() const {
    return deviceName_;
}

bool SpotifyClient::refreshToken() {
    return fetchAccessToken();
}

int SpotifyClient::getAvailableDevices(SpotifyDevice* devices, int maxDevices) {
    HttpResult result = callApiWithRetry(HttpMethod::GET, SPOTIFY_DEVICES_URL);

    if (!result.isSuccess()) {
        DEBUG_PRINT(F("Failed to get devices: "));
        DEBUG_PRINTLN(result.httpCode);
        return 0;
    }

    return parseDevicesJson(result.payload, devices, maxDevices);
}

int SpotifyClient::parseDevicesJson(const String& json, SpotifyDevice* devices, int maxDevices) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        DEBUG_PRINT(F("JSON parse error: "));
        DEBUG_PRINTLN(error.c_str());
        return 0;
    }

    JsonArray devicesArray = doc["devices"].as<JsonArray>();
    int deviceCount = 0;

    for (JsonObject device : devicesArray) {
        if (deviceCount >= maxDevices) break;

        devices[deviceCount].id = device["id"].as<String>();
        devices[deviceCount].name = device["name"].as<String>();
        devices[deviceCount].type = device["type"].as<String>();
        devices[deviceCount].isActive = device["is_active"].as<bool>();
        devices[deviceCount].isRestricted = device["is_restricted"].as<bool>();

        if (!devices[deviceCount].id.isEmpty()) {
            deviceCount++;
        }
    }

    return deviceCount;
}

String SpotifyClient::getDevicesJson() {
    const int MAX_DEVICES = 10;
    SpotifyDevice devices[MAX_DEVICES];

    int count = getAvailableDevices(devices, MAX_DEVICES);

    // Build JSON response with ArduinoJson
    JsonDocument doc;
    JsonArray devicesArray = doc.to<JsonArray>();

    for (int i = 0; i < count; i++) {
        JsonObject device = devicesArray.add<JsonObject>();
        device["id"] = devices[i].id;
        device["name"] = devices[i].name;
        device["type"] = devices[i].type;
        device["is_active"] = devices[i].isActive;
        device["is_restricted"] = devices[i].isRestricted;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

bool SpotifyClient::setDeviceById(const String& deviceId) {
    if (deviceId.isEmpty()) {
        return false;
    }

    // Verify device exists by fetching device list
    const int MAX_DEVICES = 10;
    SpotifyDevice devices[MAX_DEVICES];
    int count = getAvailableDevices(devices, MAX_DEVICES);

    for (int i = 0; i < count; i++) {
        if (devices[i].id == deviceId) {
            deviceId_ = deviceId;
            deviceName_ = devices[i].name;
            DEBUG_PRINT(F("Device set to: "));
            DEBUG_PRINTLN(deviceName_);
            return true;
        }
    }

    DEBUG_PRINTLN(F("Device ID not found in available devices"));
    return false;
}

bool SpotifyClient::setDeviceByName(const String& name) {
    const int MAX_DEVICES = 10;
    SpotifyDevice devices[MAX_DEVICES];
    int count = getAvailableDevices(devices, MAX_DEVICES);

    for (int i = 0; i < count; i++) {
        if (devices[i].name == name) {
            deviceId_ = devices[i].id;
            deviceName_ = devices[i].name;
            DEBUG_PRINT(F("Device set to: "));
            DEBUG_PRINTLN(deviceName_);
            return true;
        }
    }

    DEBUG_PRINT(F("Device '"));
    DEBUG_PRINT(name);
    DEBUG_PRINTLN(F("' not found"));
    return false;
}

HttpResult SpotifyClient::callApi(HttpMethod method, const String& url, const String& body) {
    HttpResult result;
    result.httpCode = 0;

    DEBUG_PRINT(F("API call: "));
    DEBUG_PRINTLN(url);

    HTTPClient http;
    http.begin(wifiClient_, url);
    http.addHeader(F("Content-Type"), F("application/json"));
    http.addHeader(F("Authorization"), buildBearerAuthHeader());

    // ESP8266 HTTPClient bug: Content-Length not added when body is empty
    if (body.isEmpty()) {
        http.addHeader(F("Content-Length"), "0");
    }

    switch (method) {
        case HttpMethod::GET:
            result.httpCode = http.GET();
            break;
        case HttpMethod::POST:
            result.httpCode = http.POST(body);
            break;
        case HttpMethod::PUT:
            result.httpCode = http.PUT(body);
            break;
    }

    if (result.httpCode > 0 && http.getSize() > 0) {
        result.payload = http.getString();
    }

    DEBUG_PRINT(F("Response code: "));
    DEBUG_PRINTLN(result.httpCode);

    http.end();
    return result;
}

HttpResult SpotifyClient::callApiWithRetry(HttpMethod method, const String& url, const String& body) {
    HttpResult result;
    uint8_t retryCount = 0;

    while (retryCount <= retryConfig_.maxRetries) {
        result = callApi(method, url, body);

        // Success or non-retryable error
        if (result.isSuccess() || !result.shouldRetry()) {
            currentRetryCount_ = 0;
            return result;
        }

        // Check if we should retry
        if (retryCount < retryConfig_.maxRetries) {
            uint32_t delayMs = calculateBackoffDelay(retryCount);

            DEBUG_PRINT(F("Request failed, retrying in "));
            DEBUG_PRINT(delayMs);
            DEBUG_PRINTLN(F("ms..."));

            // Non-blocking delay using yield()
            unsigned long startTime = millis();
            while (millis() - startTime < delayMs) {
                yield(); // Allow ESP8266 to handle background tasks
            }

            retryCount++;
            currentRetryCount_ = retryCount;
        } else {
            break;
        }
    }

    DEBUG_PRINT(F("Request failed after "));
    DEBUG_PRINT(retryConfig_.maxRetries);
    DEBUG_PRINTLN(F(" retries"));

    return result;
}

uint32_t SpotifyClient::calculateBackoffDelay(uint8_t retryCount) const {
    // Exponential backoff with jitter
    uint32_t delay = retryConfig_.initialDelayMs;

    for (uint8_t i = 0; i < retryCount; i++) {
        delay = static_cast<uint32_t>(delay * retryConfig_.backoffMultiplier);
        if (delay > retryConfig_.maxDelayMs) {
            delay = retryConfig_.maxDelayMs;
            break;
        }
    }

    // Add jitter (±25% randomization to prevent thundering herd)
    uint32_t jitter = random(delay / 4);
    if (random(2) == 0) {
        delay += jitter;
    } else {
        delay -= jitter;
    }

    return delay;
}

String SpotifyClient::extractDeviceId(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        DEBUG_PRINT(F("JSON parse error: "));
        DEBUG_PRINTLN(error.c_str());
        return "";
    }

    JsonArray devices = doc["devices"].as<JsonArray>();

    for (JsonObject device : devices) {
        String name = device["name"].as<String>();
        if (name == deviceName_) {
            return device["id"].as<String>();
        }
    }

    return "";
}

String SpotifyClient::buildBasicAuthHeader() const {
    String credentials = clientId_ + ":" + clientSecret_;
    return base64::encode(credentials);
}

String SpotifyClient::buildBearerAuthHeader() const {
    return "Bearer " + accessToken_;
}
