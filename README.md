# ESP8266 Spotify Player

Tap an NFC tag to play Spotify on any device. ESP8266 + MFRC522 + NeoPixel LEDs.

## Hardware

| Component | Pin |
|-----------|-----|
| NeoPixel Data | GPIO4 (D2) |
| MFRC522 SDA | GPIO15 (D8) |
| MFRC522 RST | GPIO0 (D3) |
| MFRC522 SCK | GPIO14 (D5) |
| MFRC522 MOSI | GPIO13 (D7) |
| MFRC522 MISO | GPIO12 (D6) |
| Mic (optional) | A0 |

> ⚠️ MFRC522 runs on 3.3V only!

## Setup

### 1. Spotify Credentials

1. Create app at [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
2. Add redirect URI: `http://localhost:8888/callback`
3. Get refresh token: `npx spotify-token-gen --client-id YOUR_ID --client-secret YOUR_SECRET`

Create `include/Config_local.h`:

```cpp
#ifndef CONFIG_LOCAL_H
#define CONFIG_LOCAL_H

#define SPOTIFY_CLIENT_ID       "your_client_id"
#define SPOTIFY_CLIENT_SECRET   "your_client_secret"
#define SPOTIFY_REFRESH_TOKEN   "your_refresh_token"
#define SPOTIFY_DEVICE_NAME     "Your Device Name"

#endif
```

### 2. Build & Upload

```bash
pio run -t upload && pio device monitor
```

### 3. WiFi Setup

On first boot, connect to `SpotifyPlayer-AP` (password: `configme123`) and configure WiFi.

## Writing NFC Tags

Use the NFC Tools app to write Spotify URIs:

```
spotify:playlist:37i9dQZF1DXcBWIGoYBM5M
spotify:album:ALBUM_ID
spotify:track:TRACK_ID
```

Find URIs: Right-click in Spotify → Share → Copy Spotify URI

## LED Status

| Color           | Meaning               |
| --------------- | --------------------- |
| 🌈 Rainbow sweep | Starting up           |
| 🟡 Yellow pulse  | Connecting to WiFi    |
| 🔴 Red blink     | WiFi error            |
| 🔵 Blue pulse    | Connecting to Spotify |
| 🟢 Green solid   | Ready                 |
| 🔵 Blue spin     | Reading tag           |
| 🟢 Green flash   | Success               |
| 🔴 Red flash     | Failed                |

## Web Interface

Access `http://<device-ip>` to select playback devices and manage settings.

## Troubleshooting

- **No WiFi portal**: Device may already be configured. Reset via web interface.
- **NFC not working**: Check wiring and 3.3V power.
- **Playback fails**: Ensure Spotify is open on target device, check serial logs.

## License

MIT
