# Spotify Setup Guide

This guide walks you through setting up the ESP8266 Spotify Player with the web-based configuration system.

## Overview

The device now supports **web-based configuration** - no need to hardcode any credentials! After connecting to WiFi, you can configure Spotify through a browser.

## Quick Start

### 1. Flash the Firmware

```bash
# Using PlatformIO
pio run -t upload
```

### 2. Connect to WiFi

1. On first boot, the device creates a WiFi access point named **"SpotifyPlayer-AP"**
2. Connect to it with password: **"configme123"**
3. A captive portal will open - select your home WiFi network
4. Enter your WiFi password and save

### 3. Find the Device IP

After WiFi connects, the device IP address is printed to serial:
```
╔════════════════════════════════════════════════╗
║         WEB CONFIGURATION AVAILABLE            ║
╠════════════════════════════════════════════════╣
║  Open in browser: http://192.168.1.xxx        ║
╚════════════════════════════════════════════════╝
```

You can also check your router's DHCP client list.

### 4. Create a Spotify App

1. Go to [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
2. Log in with your Spotify account
3. Click **"Create App"**
4. Fill in:
   - **App name**: Anything (e.g., "ESP8266 Player")
   - **App description**: Anything
   - **Redirect URI**: `http://<YOUR_ESP_IP>/callback` (e.g., `http://192.168.1.100/callback`)
5. Check the "I understand" checkbox
6. Click **"Save"**
7. Click **"Settings"** to find your **Client ID** and **Client Secret**

> ⚠️ **Important**: The Redirect URI must match your ESP8266's IP address exactly!

### 5. Configure via Web Interface

1. Open `http://<YOUR_ESP_IP>` in a browser
2. Enter your:
   - **Client ID** (from Spotify Dashboard)
   - **Client Secret** (from Spotify Dashboard)
   - **Device Name** (exact name of your Spotify Connect speaker/device)
3. Click **"Save & Authorize with Spotify"**
4. Log in to Spotify when redirected
5. Click **"Agree"** to authorize the app

### 6. Done!

The device is now configured and will remember your settings even after power cycles.

---

## Finding Your Spotify Device Name

The device name must match **exactly** what Spotify shows. To find it:

1. Open Spotify on your phone/computer
2. Play something
3. Tap the "Devices Available" icon (speaker icon)
4. Note the exact name of the device you want to control

Common device names:
- `Living Room Speaker`
- `Kitchen Echo`
- `SAMSUNG TV`
- `Web Player (Chrome)`

---

## Troubleshooting

### WiFi Issues

**Can't connect to saved WiFi network?**

The device will automatically:
1. Try reconnecting 3 times over 60 seconds
2. If still failing, automatically restart the WiFi setup portal
3. Connect to "SpotifyPlayer-AP" to reconfigure

**Manual WiFi Reset:**

Option 1: Connect a button to the GPIO pin defined in Config.h (`WIFI_RESET_BUTTON_PIN`) and hold for 3 seconds

Option 2: Send a serial command (if you have serial access):
```
// This will be added in a future update
```

Option 3: Re-flash the firmware with the `--erase-flash` option:
```bash
pio run -t erase
pio run -t upload
```

**WiFi keeps disconnecting:**

- The device will auto-reconnect up to 3 times
- After 60 seconds of failed reconnects, it restarts the WiFi portal
- Make sure your WiFi signal is strong enough

### Spotify Issues

### "Device not found" error

- Make sure the Spotify device is **powered on and connected**
- Play something on it first to "wake it up"
- Verify the device name matches exactly (case-sensitive!)

### "Token fetch failed" error

- Double-check your Client ID and Client Secret
- Make sure the Redirect URI in Spotify Dashboard matches exactly

### Can't access web interface

- Make sure you're on the same WiFi network as the ESP8266
- Try accessing by IP address directly
- Check serial output for the correct IP

### Authorization fails / redirect error

1. In Spotify Dashboard, go to your app's **Settings**
2. Under **Redirect URIs**, add: `http://<YOUR_ESP_IP>/callback`
3. Make sure there are no trailing slashes
4. Save and try again

### Need to reconfigure?

- Visit `http://<ESP_IP>/` to change settings
- Or POST to `http://<ESP_IP>/clear` to reset everything

---

## How It Works

### Authentication Flow

```
┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│   Browser   │     │   ESP8266    │     │   Spotify   │
└─────┬───────┘     └──────┬───────┘     └──────┬──────┘
      │                    │                    │
      │  1. Enter creds    │                    │
      │───────────────────>│                    │
      │                    │                    │
      │  2. Redirect       │                    │
      │<───────────────────│                    │
      │                    │                    │
      │  3. User login     │                    │
      │────────────────────────────────────────>│
      │                    │                    │
      │  4. Auth code      │                    │
      │<────────────────────────────────────────│
      │                    │                    │
      │  5. Callback       │                    │
      │───────────────────>│                    │
      │                    │                    │
      │                    │  6. Exchange code  │
      │                    │───────────────────>│
      │                    │                    │
      │                    │  7. Refresh token  │
      │                    │<───────────────────│
      │                    │                    │
      │  8. Success        │                    │
      │<───────────────────│                    │
```

### Token Refresh

The **refresh token** is stored in flash memory (LittleFS) and used to automatically get new **access tokens** whenever they expire (every hour). You only need to authorize once!

---

## Security Notes

- Credentials are stored in plaintext on the ESP8266's flash
- The web interface is accessible to anyone on your local network
- Consider disabling the web server after configuration if security is a concern

To disable the web server after setup, you could modify the code to only run it when a button is pressed or during a specific time after boot.

---

## API Reference

### Web Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Configuration page |
| `/save` | POST | Save credentials and start OAuth |
| `/callback` | GET | OAuth callback from Spotify |
| `/status` | GET | JSON status of configuration |
| `/clear` | POST | Clear all saved configuration |
