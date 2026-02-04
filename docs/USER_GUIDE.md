# ESP8266 Spotify Player - User Guide

Welcome to the ESP8266 Spotify Player! This guide will help you understand how to use your device effectively and interpret the LED ring status indicators.

## Table of Contents

- [Getting Started](#getting-started)
- [LED Ring Status Guide](#led-ring-status-guide)
- [Using NFC Tags](#using-nfc-tags)
- [Web Interface](#web-interface)
- [Troubleshooting](#troubleshooting)

---

## Getting Started

### First-Time Setup

1. **Power on the device** - Connect your ESP8266 to a USB power source
2. **Watch for startup animation** - The LED ring will display a rainbow sweep
3. **Connect to WiFi** - If not configured, the device creates an access point:
   - Network name: `SpotifyPlayer-AP`
   - Password: `configme123`
4. **Configure WiFi** - Open a browser and go to `192.168.4.1` to select your WiFi network
5. **Wait for connection** - The ring will pulse yellow while connecting

### Normal Operation

Once configured, simply tap an NFC tag with a Spotify URI to start playback!

---

## LED Ring Status Guide

The LED ring provides visual feedback about the device's current state. Here's what each animation means:

### 🚀 Startup & Connection States

| Animation | Color | Pattern | Meaning |
|-----------|-------|---------|---------|
| **Startup** | 🌈 Rainbow | Sweeping fill | Device is booting up |
| **WiFi Connecting** | 🟡 Yellow | Pulsing (breathing) | Connecting to WiFi network |
| **WiFi Error** | 🔴 Red | Fast blinking | Cannot connect to WiFi |
| **Spotify Connecting** | 🔵 Blue | Pulsing (breathing) | Authenticating with Spotify |
| **Spotify Error** | 🟠 Orange | Slow blinking | Spotify connection issue |
| **Token Refresh** | 🟣 Purple | Pulsing | Refreshing Spotify authentication |

### 📖 NFC Reading States

| Animation | Color | Pattern | Meaning |
|-----------|-------|---------|---------|
| **NFC Reading** | 🔵 Blue | Spinning (comet) | Reading NFC tag data || **Tag Processing** | 🟣 Blue-Purple | Dual spin (opposite) | Sending to Spotify API || **Tag Success** | 🟢 Green | 3 quick flashes | Tag read successfully, playing! |
| **Tag Failure** | 🔴 Red | 5 quick flashes | Failed to read or play tag |

### 🎵 Playback States

| Animation | Color | Pattern | Meaning |
|-----------|-------|---------|---------|
| **Music Playing** | 🌈 Rainbow | Smooth wave | Music is currently playing |
| **Music Paused** | 🔵 Dim Blue | Slow breathing | Playback is paused |
| **Skip Track** | 🩵 Cyan | Chase right (→) | Skipping to next track |
| **Previous Track** | 🩵 Cyan | Chase left (←) | Going to previous track |
| **Volume Up** | 🟢 Teal | Expanding ring | Volume increased |
| **Volume Down** | 🟢 Teal | Contracting ring | Volume decreased |

### 📡 Device & System States

| Animation | Color | Pattern | Meaning |
|-----------|-------|---------|---------|
| **Idle/Ready** | 🟢 Dim Green | Solid (sound reactive) | Ready to scan tags |
| **Device Selected** | 🩵 Cyan | 2 quick flashes | Playback device changed |
| **Searching** | ⚪ White | Sparkle/twinkle | Searching for Spotify devices |
| **Standby** | ⚪ Very Dim White | Very slow breathing | Low-power standby mode |

---

## Visual Animation Reference

### Pulsing / Breathing
```
    ●●●●●●●●    (bright)
       ↕
    ○○○○○○○○    (dim)
```
The entire ring smoothly fades between bright and dim states.

### Spinning / Comet
```
    ●●○○○○○○ → ○●●○○○○○ → ○○●●○○○○ → ...
```
A bright "head" with a fading tail rotates around the ring.

### Dual Spinning (Processing)
```
    Comet 1 (clockwise):        ●●○○○○○○ → ○●●○○○○○
    Comet 2 (counter-clockwise): ○○○○○○●● → ○○○○○●●○
```
Two comets with different colors spinning in opposite directions, indicating active processing/communication with Spotify API.

### Blinking
```
    ●●●●●●●●    (on)
       ↕
    ○○○○○○○○    (off)
```
The entire ring turns on and off. Fast = error, Slow = warning.

### Rainbow Wave
```
    🔴🟠🟡🟢🔵🟣🔴🟠 → 🟠🟡🟢🔵🟣🔴🟠🟡 → ...
```
Colors smoothly shift around the ring in a continuous rainbow pattern.

### Expanding Ring
```
    ○○○●●○○○ → ○○●●●●○○ → ○●●●●●●○ → ●●●●●●●●
```
Light expands outward from the center.

### Contracting Ring
```
    ●●●●●●●● → ○●●●●●●○ → ○○●●●●○○ → ○○○●●○○○
```
Light contracts inward toward the center.

### Chase
```
    →→→ (right): ●○○○○○○○ → ○●○○○○○○ → ○○●○○○○○ → ...
    ←←← (left):  ○○○○○○○● → ○○○○○○●○ → ○○○○○●○○ → ...
```
A comet-like trail moves around the ring in one direction.

### Sparkle / Twinkle
```
    ○●○○○○●○ → ○○●○○●○○ → ●○○○●○○○ → ...
```
Random LEDs light up briefly, creating a twinkling effect.

---

## Sound Reactive Mode

When in **Idle/Ready** state, the LED ring responds to ambient sound:

- 🎤 The microphone picks up audio levels
- 💡 LEDs pulse with the music/sound intensity
- 🟢 Brightness varies from dim to bright green based on volume

This creates a fun visualization that "dances" with the music!

---

## Using NFC Tags

### Supported Tag Types

- NTAG213, NTAG215, NTAG216
- MIFARE Classic 1K
- Most NFC-enabled cards and stickers

### Writing Spotify URIs to Tags

1. Install **NFC Tools** app on your smartphone
2. Open Spotify and find the content you want
3. Tap "Share" → "Copy Link" or get the URI
4. In NFC Tools:
   - Go to **Write** tab
   - Add Record → **URI**
   - Paste the Spotify URI (e.g., `spotify:playlist:37i9dQZF1DXcBWIGoYBM5M`)
   - Hold tag to your phone to write

### URI Formats

| Content Type | URI Format | Example |
|--------------|------------|---------|
| Playlist | `spotify:playlist:ID` | `spotify:playlist:37i9dQZF1DXcBWIGoYBM5M` |
| Album | `spotify:album:ID` | `spotify:album:4aawyAB9vmqN3uQ7FjRGTy` |
| Artist | `spotify:artist:ID` | `spotify:artist:0OdUWJ0sBjDrqHygGUXeCF` |
| Track | `spotify:track:ID` | `spotify:track:7qiZfU4dY1lWllzX7mPBI3` |
| Podcast | `spotify:show:ID` | `spotify:show:4rOoJ6Egrf8K2IrywzwOMk` |
| Episode | `spotify:episode:ID` | `spotify:episode:0Q86acNRm6V9GYx55SXKwf` |

### Finding Spotify URIs

**Desktop App:**
1. Right-click on any item
2. Share → Copy Spotify URI

**Mobile App:**
1. Tap the three dots (•••)
2. Share → Copy link
3. Convert the link to URI format

---

## Web Interface

Access the web interface by navigating to the device's IP address in your browser.

### Features

- **Device Selection** - Choose which Spotify Connect device to play on
- **Current Status** - View connection and playback status
- **Manual Control** - Basic playback controls (if implemented)

### Finding the IP Address

The IP address is printed to the serial monitor on startup. Typical format: `192.168.x.x`

---

## Troubleshooting

### LED Ring Shows Constant Red Blinking
**Problem:** Cannot connect to WiFi
**Solutions:**
1. Check WiFi credentials
2. Move device closer to router
3. Restart the device
4. Reset WiFi settings by holding reset during boot

### LED Ring Shows Orange Blinking
**Problem:** Spotify authentication issue
**Solutions:**
1. Check your Spotify credentials in `Config_local.h`
2. Regenerate your refresh token
3. Ensure your Spotify Premium subscription is active
4. Open Spotify on a device to activate your account

### NFC Tag Not Reading
**Problem:** Blue spinning continues or red flashes
**Solutions:**
1. Hold the tag flat against the reader
2. Try different tag positions
3. Ensure the tag contains a valid Spotify URI
4. Check NFC reader wiring connections

### No Sound After Tag Read
**Problem:** Green flash but no playback
**Solutions:**
1. Ensure a Spotify device is active and selected
2. Check the web interface to select a device
3. Open Spotify app on the target device first
4. Verify the Spotify URI is valid

### Rainbow Wave but No Music
**Problem:** Device shows playing but no sound
**Solutions:**
1. Check volume on the target Spotify device
2. Ensure the correct playback device is selected
3. The content might be unavailable in your region

---

## Quick Reference Card

Print this out and keep it near your device!

| LED Pattern | What It Means | What To Do |
|-------------|---------------|------------|
| 🌈 Rainbow sweep | Starting up | Wait ~5 seconds |
| 🟡 Yellow pulse | Connecting WiFi | Wait for connection |
| 🔵 Blue pulse | Connecting Spotify | Wait for auth |
| 🟢 Dim green solid | Ready! | Scan a tag |
| 🔵 Blue spinning | Reading tag | Hold tag steady |
| � Blue-Purple dual spin | Processing tag | Wait for playback |
| �🟢 Green flashes | Success! | Enjoy your music |
| 🌈 Rainbow wave | Music playing | 🎵 |
| 🔴 Red fast blink | WiFi error | Check network |
| 🟠 Orange blink | Spotify error | Check credentials |
| 🔴 Red flashes | Tag failed | Try again |

---

## Tips & Tricks

1. **Quick Tag Test**: Write a known-working URI to a test tag to verify the system is working

2. **Organize Your Tags**: Use colored stickers or cases to organize playlists by genre or mood

3. **Guest Mode**: Share the WiFi AP password with guests so they can add their own tags

4. **Placement**: Position the device where the LED ring is visible for status feedback

5. **Sound Reactive**: For best effect in idle mode, place near speakers so the mic can pick up the music

---

*Happy listening! 🎵*
