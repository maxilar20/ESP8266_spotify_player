# API Documentation

This document provides detailed documentation for the ESP8266 Spotify Player codebase.

## Module Overview

```
┌─────────────────────────────────────────────────────────────┐
│                         main.cpp                            │
│                    Application Entry Point                  │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │
│  │ LedController│  │  NfcReader  │  │   SpotifyClient    │ │
│  │             │  │             │  │                     │ │
│  │ - Status    │  │ - Tag       │  │ - Authentication    │ │
│  │ - Animation │  │   Reading   │  │ - Playback Control  │ │
│  │ - Sound     │  │ - URI       │  │ - Device Discovery  │ │
│  │   Reactive  │  │   Parsing   │  │                     │ │
│  └─────────────┘  └─────────────┘  └─────────────────────┘ │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│                        Config.h                             │
│                 Configuration Constants                     │
└─────────────────────────────────────────────────────────────┘
```

---

## LedController

Controls the NeoPixel LED strip for visual feedback and effects.

### Class Definition

```cpp
class LedController {
public:
    LedController(uint8_t pin, uint16_t numLeds);
    void begin();
    void clear();
    void setBrightness(uint8_t brightness);
    void setColor(uint8_t r, uint8_t g, uint8_t b, bool animate = true);
    void showConnecting();
    void showSuccess();
    void showReading();
    void showError();
    void updateSoundReactive(int audioLevel);
    void update();
};
```

### Constructor

```cpp
LedController(uint8_t pin, uint16_t numLeds)
```

Creates a new LED controller instance.

**Parameters:**
- `pin` - GPIO pin connected to NeoPixel data line
- `numLeds` - Number of LEDs in the strip

**Example:**
```cpp
LedController leds(4, 8);  // GPIO4, 8 LEDs
```

### Methods

#### begin()
```cpp
void begin()
```
Initializes the LED strip. Must be called in `setup()`.

#### setColor()
```cpp
void setColor(uint8_t r, uint8_t g, uint8_t b, bool animate = true)
```
Sets all LEDs to a solid color.

**Parameters:**
- `r` - Red component (0-255)
- `g` - Green component (0-255)
- `b` - Blue component (0-255)
- `animate` - If true, LEDs light up sequentially with delay

#### Status Methods
```cpp
void showConnecting()  // Yellow - WiFi connecting
void showSuccess()     // Green - Operation successful
void showReading()     // Blue - Reading NFC
void showError()       // Red - Error occurred
```

#### updateSoundReactive()
```cpp
void updateSoundReactive(int audioLevel)
```
Updates the sound-reactive visualization.

**Parameters:**
- `audioLevel` - Raw analog reading (0-1023) from microphone

---

## NfcReader

Handles NFC tag detection and Spotify URI extraction.

### Class Definition

```cpp
class NfcReader {
public:
    NfcReader(uint8_t ssPin, uint8_t rstPin);
    bool begin();
    bool isNewCardPresent();
    NfcReadResult readSpotifyUri();
    void haltCard();
};
```

### Structs

#### NfcReadResult
```cpp
struct NfcReadResult {
    bool success;
    String spotifyUri;
    String errorMessage;
};
```

### Constructor

```cpp
NfcReader(uint8_t ssPin, uint8_t rstPin)
```

**Parameters:**
- `ssPin` - SPI Slave Select pin
- `rstPin` - Reset pin

### Methods

#### begin()
```cpp
bool begin()
```
Initializes the MFRC522 reader.

**Returns:** `true` if initialization successful

#### isNewCardPresent()
```cpp
bool isNewCardPresent()
```
Checks for a new NFC card in range.

**Returns:** `true` if a new card is detected and ready to read

#### readSpotifyUri()
```cpp
NfcReadResult readSpotifyUri()
```
Reads and parses the Spotify URI from the current card.

**Returns:** `NfcReadResult` with success status and parsed URI

**Example:**
```cpp
if (nfcReader.isNewCardPresent()) {
    NfcReadResult result = nfcReader.readSpotifyUri();
    if (result.success) {
        Serial.println(result.spotifyUri);
    }
}
```

### Tag Format

The reader expects NFC tags with NDEF records containing Spotify URLs. The URL is automatically converted to a Spotify URI format:

| Input URL | Output URI |
|-----------|------------|
| `open.spotify.com/playlist/xxx` | `spotify:playlist:xxx` |
| `open.spotify.com/album/xxx` | `spotify:album:xxx` |

---

## SpotifyClient

Handles all Spotify Web API interactions.

### Class Definition

```cpp
class SpotifyClient {
public:
    SpotifyClient(
        const String& clientId,
        const String& clientSecret,
        const String& deviceName,
        const String& refreshToken
    );

    bool begin();
    bool fetchAccessToken();
    bool discoverDevice();
    bool playUri(const String& contextUri);
    bool nextTrack();
    bool enableShuffle();
    bool isAuthenticated() const;
    bool isDeviceAvailable() const;
};
```

### Structs

#### HttpResult
```cpp
struct HttpResult {
    int httpCode;
    String payload;

    bool isSuccess() const;
    bool isUnauthorized() const;
    bool isNotFound() const;
};
```

### Constructor

```cpp
SpotifyClient(
    const String& clientId,
    const String& clientSecret,
    const String& deviceName,
    const String& refreshToken
)
```

**Parameters:**
- `clientId` - Spotify application Client ID
- `clientSecret` - Spotify application Client Secret
- `deviceName` - Name of target Spotify Connect device
- `refreshToken` - OAuth refresh token

### Methods

#### begin()
```cpp
bool begin()
```
Initializes the client, fetches access token, and discovers device.

**Returns:** `true` if fully initialized

#### fetchAccessToken()
```cpp
bool fetchAccessToken()
```
Fetches a new access token using the refresh token.

**Returns:** `true` if token obtained successfully

#### discoverDevice()
```cpp
bool discoverDevice()
```
Queries Spotify API for available devices and finds the target device.

**Returns:** `true` if target device found

#### playUri()
```cpp
bool playUri(const String& contextUri)
```
Starts playback of a Spotify URI.

**Parameters:**
- `contextUri` - Spotify URI (e.g., `spotify:playlist:xxx`)

**Returns:** `true` if playback started

**Notes:**
- Automatically enables shuffle mode
- Handles token expiration and device unavailability

---

## Configuration

All configuration is centralized in `Config.h`.

### Hardware Configuration

```cpp
// LED Strip
#define LED_PIN             4       // GPIO pin
#define NUM_LEDS            8       // LED count
#define LED_BRIGHTNESS      50      // 0-255

// NFC Reader
#define NFC_RST_PIN         0       // Reset pin
#define NFC_SS_PIN          15      // SPI SS pin

// Microphone
#define MIC_PIN             A0      // Analog input
#define MIC_SAMPLE_INTERVAL 35      // ms between samples
```

### Network Configuration

```cpp
#define WIFI_AP_NAME        "SpotifyPlayer-AP"
#define WIFI_AP_PASSWORD    "configme123"
#define WIFI_RECONNECT_DELAY 20000  // ms
```

### Spotify Configuration

```cpp
#define SPOTIFY_CLIENT_ID       "your_client_id"
#define SPOTIFY_CLIENT_SECRET   "your_client_secret"
#define SPOTIFY_REFRESH_TOKEN   "your_refresh_token"
#define SPOTIFY_DEVICE_NAME     "Your Device"
```

### Debug Macros

```cpp
#define DEBUG_ENABLED  // Comment out to disable debug output

DEBUG_PRINT(x)      // Serial.print equivalent
DEBUG_PRINTLN(x)    // Serial.println equivalent
DEBUG_PRINTF(...)   // Serial.printf equivalent
```

---

## Error Handling

### HTTP Response Codes

| Code | Meaning | Action |
|------|---------|--------|
| 200-299 | Success | Continue |
| 401 | Unauthorized | Refresh token |
| 404 | Device not found | Rediscover device |
| 429 | Rate limited | Wait and retry |
| 5xx | Server error | Retry later |

### Common Issues

1. **Token Expiration** - Automatically handled by `playUri()`
2. **Device Unavailable** - Rediscovers device on 404
3. **WiFi Disconnect** - Automatic reconnection in main loop

---

## Extending the Code

### Adding New LED Effects

1. Add method declaration to `LedController.h`
2. Implement in `LedController.cpp`
3. Call from `main.cpp`

Example:
```cpp
// In LedController.h
void rainbow();

// In LedController.cpp
void LedController::rainbow() {
    for (uint16_t i = 0; i < numLeds_; i++) {
        pixels_.setPixelColor(i, Wheel((i * 256 / numLeds_) & 255));
    }
    pixels_.show();
}
```

### Adding New Spotify Commands

1. Add method to `SpotifyClient.h`
2. Implement API call in `SpotifyClient.cpp`

Example:
```cpp
// Add pause functionality
bool SpotifyClient::pause() {
    String url = String(SPOTIFY_API_BASE_URL) + "/me/player/pause";
    HttpResult result = callApi(HttpMethod::PUT, url);
    return result.isSuccess();
}
```
