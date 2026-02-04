# ESP8266 Spotify Player

[![PlatformIO CI](https://github.com/yourusername/ESP8266_spotify_player/actions/workflows/build.yml/badge.svg)](https://github.com/yourusername/ESP8266_spotify_player/actions/workflows/build.yml)

An ESP8266-based NFC-controlled Spotify player. Tap an NFC tag to instantly play your favorite playlists, albums, or podcasts on any Spotify Connect device.

![Project Banner](docs/images/banner.png)

## Features

- 🎵 **NFC Tag Control** - Tap NFC tags to play Spotify content
- 📶 **WiFi Manager** - Easy WiFi setup via captive portal
- 💡 **LED Status Indicators** - Visual feedback with NeoPixel strip
- 🎤 **Sound Reactive Mode** - LED visualization responding to audio
- 🔄 **Auto-reconnect** - Resilient WiFi and Spotify connection handling

## Hardware Requirements

| Component | Description |
|-----------|-------------|
| ESP8266 | NodeMCU v2 or compatible board |
| MFRC522 | NFC/RFID reader module |
| WS2812B | NeoPixel LED strip (8 LEDs recommended) |
| Microphone | Analog sound sensor module (optional) |

### Pin Connections

| Component | ESP8266 Pin |
|-----------|-------------|
| NeoPixel Data | GPIO4 (D2) |
| MFRC522 SDA | GPIO15 (D8) |
| MFRC522 RST | GPIO0 (D3) |
| MFRC522 SCK | GPIO14 (D5) |
| MFRC522 MOSI | GPIO13 (D7) |
| MFRC522 MISO | GPIO12 (D6) |
| Microphone | A0 |

## Software Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- [Spotify Developer Account](https://developer.spotify.com/)

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/ESP8266_spotify_player.git
cd ESP8266_spotify_player
```

### 2. Configure Spotify Credentials

1. Go to [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
2. Create a new application
3. Add `http://localhost:8888/callback` as a redirect URI
4. Copy the Client ID and Client Secret

Create a `include/Config_local.h` file with your credentials:

```cpp
#ifndef CONFIG_LOCAL_H
#define CONFIG_LOCAL_H

#define SPOTIFY_CLIENT_ID       "your_client_id"
#define SPOTIFY_CLIENT_SECRET   "your_client_secret"
#define SPOTIFY_REFRESH_TOKEN   "your_refresh_token"
#define SPOTIFY_DEVICE_NAME     "Your Device Name"

#endif
```

### 3. Get Refresh Token

Use a tool like [spotify-token-gen](https://github.com/bkeepers/spotify-token-gen) to get your refresh token:

```bash
npx spotify-token-gen --client-id YOUR_CLIENT_ID --client-secret YOUR_CLIENT_SECRET
```

### 4. Build and Upload

```bash
# Build firmware
pio run

# Upload to ESP8266
pio run -t upload

# Monitor serial output
pio device monitor
```

## Configuration

All configuration options are in `include/Config.h`:

| Setting | Default | Description |
|---------|---------|-------------|
| `LED_PIN` | 4 | GPIO pin for NeoPixel data |
| `NUM_LEDS` | 8 | Number of LEDs in strip |
| `LED_BRIGHTNESS` | 50 | Default brightness (0-255) |
| `NFC_RST_PIN` | 0 | NFC reader reset pin |
| `NFC_SS_PIN` | 15 | NFC reader SPI SS pin |
| `WIFI_AP_NAME` | "SpotifyPlayer-AP" | Access point name for setup |

## Writing NFC Tags

NFC tags should contain a Spotify URI in NDEF format. The supported URI formats are:

- `spotify:album:ALBUM_ID`
- `spotify:playlist:PLAYLIST_ID`
- `spotify:artist:ARTIST_ID`
- `spotify:track:TRACK_ID`

### Using NFC Tools App

1. Open NFC Tools app on your phone
2. Go to "Write" tab
3. Add a record → URI
4. Enter the Spotify URI (e.g., `spotify:playlist:37i9dQZF1DXcBWIGoYBM5M`)
5. Write to tag

### Finding Spotify URIs

1. Open Spotify desktop app
2. Right-click on any album/playlist/track
3. Share → Copy Spotify URI

## Project Structure

```
ESP8266_spotify_player/
├── .github/
│   └── workflows/
│       └── build.yml          # CI/CD pipeline
├── docs/
│   ├── images/               # Documentation images
│   ├── API.md               # Code documentation
│   ├── HARDWARE.md          # Hardware assembly guide
│   ├── SPOTIFY_SETUP.md     # Spotify configuration guide
│   └── USER_GUIDE.md        # Complete user guide with LED status
├── include/
│   ├── Config.h             # Configuration settings
│   ├── Config_local.h       # Your local credentials (gitignored)
│   ├── LedController.h      # LED controller interface
│   ├── NfcReader.h          # NFC reader interface
│   ├── SpotifyClient.h      # Spotify API client interface
│   └── WebServer.h          # Web server interface
├── src/
│   ├── main.cpp             # Main application
│   ├── LedController.cpp    # LED controller implementation
│   ├── NfcReader.cpp        # NFC reader implementation
│   ├── SpotifyClient.cpp    # Spotify API client implementation
│   └── WebServer.cpp        # Web server implementation
├── platformio.ini           # PlatformIO configuration
└── README.md               # This file
```

## LED Status Indicators

The LED ring provides rich visual feedback for all device states:

### Connection States
| Animation       | Color      | Meaning               |
| --------------- | ---------- | --------------------- |
| 🌈 Rainbow Sweep | Multicolor | Device starting up    |
| 🟡 Pulsing       | Yellow     | Connecting to WiFi    |
| 🔴 Fast Blink    | Red        | WiFi connection error |
| 🔵 Pulsing       | Blue       | Connecting to Spotify |
| 🟠 Slow Blink    | Orange     | Spotify error         |
| 🟣 Pulsing       | Purple     | Refreshing token      |

### Operation States
| Animation        | Color       | Meaning                 |
| ---------------- | ----------- | ----------------------- |
| 🟢 Solid/Reactive | Dim Green   | Ready - waiting for NFC |
| 🔵 Spinning       | Blue        | Reading NFC tag         |
| � Dual Spin      | Blue-Purple | Processing tag          |
| �🟢 Flash (×3)    | Green       | Tag success - playing!  |
| 🔴 Flash (×5)     | Red         | Tag read failed         |
| 🩵 Flash (×2)     | Cyan        | Device selected         |
| ⚪ Sparkle        | White       | Searching for devices   |

### Playback States
| Animation   | Color    | Meaning            |
| ----------- | -------- | ------------------ |
| 🌈 Wave      | Rainbow  | Music playing      |
| 🔵 Breathing | Dim Blue | Music paused       |
| 🩵 Chase →   | Cyan     | Skip to next track |
| 🩵 Chase ←   | Cyan     | Previous track     |
| 🟢 Expand    | Teal     | Volume up          |
| 🟢 Contract  | Teal     | Volume down        |

📖 **See [docs/USER_GUIDE.md](docs/USER_GUIDE.md) for detailed LED animations and complete usage instructions.**

## Troubleshooting

### WiFi Connection Issues

1. Press reset button to restart
2. If captive portal doesn't appear, the device might already be configured
3. To reset WiFi settings, uncomment and call `wifiManager.resetSettings()` in setup

### NFC Reader Not Detected

1. Check SPI wiring connections
2. Ensure proper power supply (3.3V)
3. Verify the MFRC522 is genuine (some clones have issues)

### Spotify Playback Issues

1. Verify credentials in `Config_local.h`
2. Ensure the target device is active on Spotify
3. Check serial monitor for API error messages
4. Refresh token may expire - regenerate if needed

## API Reference

See [docs/API.md](docs/API.md) for detailed code documentation.

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Spotify Web API](https://developer.spotify.com/documentation/web-api/)
- [MFRC522 Arduino Library](https://github.com/miguelbalboa/rfid)
- [Adafruit NeoPixel Library](https://github.com/adafruit/Adafruit_NeoPixel)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
