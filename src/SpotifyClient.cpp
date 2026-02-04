/**
 * @file SpotifyClient.cpp
 * @brief Implementation of Spotify Web API Client
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
    accessToken_ = parseJsonValue("access_token", payload);

    http.end();

    if (accessToken_.isEmpty()) {
        DEBUG_PRINTLN(F("Failed to parse access token from response"));
        return false;
    }

    DEBUG_PRINTLN(F("Successfully obtained access token"));
    return true;
}

bool SpotifyClient::discoverDevice() {
    HttpResult result = callApi(HttpMethod::GET, SPOTIFY_DEVICES_URL);

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

    String body = "{\"context_uri\":\"" + contextUri + "\",\"offset\":{\"position\":0,\"position_ms\":0}}";
    String url = String(SPOTIFY_PLAY_URL) + "?device_id=" + deviceId_;

    HttpResult result = callApi(HttpMethod::PUT, url, body);

    // Handle specific error cases
    if (result.isNotFound()) {
        DEBUG_PRINTLN(F("Device not found, rediscovering..."));
        if (discoverDevice()) {
            url = String(SPOTIFY_PLAY_URL) + "?device_id=" + deviceId_;
            result = callApi(HttpMethod::PUT, url, body);
        }
    } else if (result.isUnauthorized()) {
        DEBUG_PRINTLN(F("Token expired, refreshing..."));
        if (fetchAccessToken()) {
            result = callApi(HttpMethod::PUT, url, body);
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
    HttpResult result = callApi(HttpMethod::POST, url);

    return result.isSuccess();
}

bool SpotifyClient::enableShuffle() {
    DEBUG_PRINTLN(F("Enabling shuffle"));

    String url = String(SPOTIFY_SHUFFLE_URL) + "?state=true&device_id=" + deviceId_;
    HttpResult result = callApi(HttpMethod::PUT, url);

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
    HttpResult result = callApi(HttpMethod::GET, SPOTIFY_DEVICES_URL);

    if (!result.isSuccess()) {
        DEBUG_PRINT(F("Failed to get devices: "));
        DEBUG_PRINTLN(result.httpCode);
        return 0;
    }

    // Parse devices from JSON
    int deviceCount = 0;
    String json = result.payload;

    // Find "devices" array
    int devicesStart = json.indexOf("\"devices\"");
    if (devicesStart < 0) return 0;

    int arrayStart = json.indexOf('[', devicesStart);
    if (arrayStart < 0) return 0;

    int pos = arrayStart + 1;

    while (deviceCount < maxDevices) {
        // Find next device object
        int objStart = json.indexOf('{', pos);
        if (objStart < 0) break;

        int objEnd = json.indexOf('}', objStart);
        if (objEnd < 0) break;

        String deviceObj = json.substring(objStart, objEnd + 1);

        // Extract device fields
        devices[deviceCount].id = parseJsonValue("id", deviceObj);
        devices[deviceCount].name = parseJsonValue("name", deviceObj);
        devices[deviceCount].type = parseJsonValue("type", deviceObj);

        // Parse boolean fields
        String isActiveStr = parseJsonValue("is_active", deviceObj);
        devices[deviceCount].isActive = (isActiveStr == "true");

        String isRestrictedStr = parseJsonValue("is_restricted", deviceObj);
        devices[deviceCount].isRestricted = (isRestrictedStr == "true");

        if (!devices[deviceCount].id.isEmpty()) {
            deviceCount++;
        }

        pos = objEnd + 1;
    }

    return deviceCount;
}

String SpotifyClient::getDevicesJson() {
    const int MAX_DEVICES = 10;
    SpotifyDevice devices[MAX_DEVICES];

    int count = getAvailableDevices(devices, MAX_DEVICES);

    String json = "[";
    for (int i = 0; i < count; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"id\":\"" + devices[i].id + "\",";
        json += "\"name\":\"" + devices[i].name + "\",";
        json += "\"type\":\"" + devices[i].type + "\",";
        json += "\"is_active\":" + String(devices[i].isActive ? "true" : "false") + ",";
        json += "\"is_restricted\":" + String(devices[i].isRestricted ? "true" : "false");
        json += "}";
    }
    json += "]";

    return json;
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

String SpotifyClient::parseJsonValue(const String& key, const String& json) {
    String value;

    // Search for quoted key pattern: "key":
    String searchPattern = "\"" + key + "\"";
    int index = json.indexOf(searchPattern);

    if (index < 0) {
        return value;
    }

    // Find the colon after the key
    int colonPos = json.indexOf(':', index);
    if (colonPos < 0) return value;

    // Skip whitespace after colon
    int valueStart = colonPos + 1;
    while (valueStart < (int)json.length() && (json.charAt(valueStart) == ' ' || json.charAt(valueStart) == '\t')) {
        valueStart++;
    }

    if (valueStart >= (int)json.length()) return value;

    // Check if value is quoted
    if (json.charAt(valueStart) == '"') {
        // String value - extract until closing quote
        valueStart++; // Skip opening quote
        for (int i = valueStart; i < (int)json.length(); i++) {
            char c = json.charAt(i);
            if (c == '"' && (i == valueStart || json.charAt(i-1) != '\\')) {
                // Found closing quote (not escaped)
                break;
            }
            value += c;
        }
    } else {
        // Non-string value (number, boolean, null) - extract until comma or }
        for (int i = valueStart; i < (int)json.length(); i++) {
            char c = json.charAt(i);
            if (c == ',' || c == '}' || c == ']' || c == ' ' || c == '\n' || c == '\r') {
                break;
            }
            value += c;
        }
    }

    return value;
}

String SpotifyClient::extractDeviceId(const String& json) {
    String id;

    // Find position of the device name
    int nameIndex = json.indexOf(deviceName_);
    if (nameIndex < 0) {
        return id;
    }

    // Back up to find the start of this device object
    int objectStart = nameIndex;
    for (; objectStart > 0; objectStart--) {
        if (json.charAt(objectStart) == '{') {
            break;
        }
    }

    // Find "id" key within this object
    int i = objectStart;
    for (; i < static_cast<int>(json.length()); i++) {
        // Stop if we hit the end of this object
        if (json.charAt(i) == '}') {
            break;
        }

        // Look for "id" pattern
        if (i + 3 < static_cast<int>(json.length()) &&
            json.charAt(i) == '"' &&
            json.charAt(i + 1) == 'i' &&
            json.charAt(i + 2) == 'd' &&
            json.charAt(i + 3) == '"') {
            i += 4;
            break;
        }
    }

    // Move to the value (skip : and opening ")
    for (; i < static_cast<int>(json.length()); i++) {
        if (json.charAt(i) == '"') {
            i++;
            break;
        }
    }

    // Extract the ID value
    for (; i < static_cast<int>(json.length()); i++) {
        char c = json.charAt(i);
        if (c == '"') {
            break;
        }
        id += c;
    }

    return id;
}

String SpotifyClient::buildBasicAuthHeader() const {
    String credentials = clientId_ + ":" + clientSecret_;
    return base64::encode(credentials);
}

String SpotifyClient::buildBearerAuthHeader() const {
    return "Bearer " + accessToken_;
}
