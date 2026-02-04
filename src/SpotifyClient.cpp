/**
 * @file SpotifyClient.cpp
 * @brief Implementation of Spotify Web API Client
 */

#include "SpotifyClient.h"
#include <ESP8266HTTPClient.h>
#include <base64.h>

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
    int index = json.indexOf(key);

    if (index < 0) {
        return value;
    }

    bool copying = false;
    for (unsigned int i = index; i < json.length(); i++) {
        if (copying) {
            char c = json.charAt(i);
            if (c == '"' || c == ',') {
                break;
            }
            value += c;
        } else if (json.charAt(i) == ':') {
            copying = true;
            // Skip opening quote if present
            if (i + 1 < json.length() && json.charAt(i + 1) == '"') {
                i++;
            }
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
