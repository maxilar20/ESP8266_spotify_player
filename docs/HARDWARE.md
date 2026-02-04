# Hardware Assembly Guide

This guide covers the hardware setup for the ESP8266 Spotify Player.

## Components List

### Required Components

| Component | Quantity | Notes |
|-----------|----------|-------|
| NodeMCU ESP8266 | 1 | v2 or v3 recommended |
| MFRC522 NFC Module | 1 | 13.56 MHz RFID reader |
| WS2812B LED Strip | 1 | 8 LEDs minimum |
| NTAG215 NFC Tags | As needed | Compatible with most smartphones |
| Micro USB Cable | 1 | For power and programming |

### Optional Components

| Component | Quantity | Notes |
|-----------|----------|-------|
| Sound Sensor Module | 1 | KY-037 or similar |
| 3D Printed Case | 1 | STL files in `hardware/` |
| Breadboard | 1 | For prototyping |
| Dupont Wires | As needed | Male-to-female recommended |

## Wiring Diagram

```
                    NodeMCU ESP8266
                    ┌─────────────┐
                    │             │
    MFRC522 SDA ────┤ D8 (GPIO15) │
    MFRC522 SCK ────┤ D5 (GPIO14) │
    MFRC522 MOSI ───┤ D7 (GPIO13) │
    MFRC522 MISO ───┤ D6 (GPIO12) │
    MFRC522 RST ────┤ D3 (GPIO0)  │
    MFRC522 3.3V ───┤ 3.3V        │
    MFRC522 GND ────┤ GND         │
                    │             │
    NeoPixel DIN ───┤ D2 (GPIO4)  │
    NeoPixel 5V ────┤ VIN/5V      │
    NeoPixel GND ───┤ GND         │
                    │             │
    Mic OUT ────────┤ A0          │
    Mic VCC ────────┤ 3.3V        │
    Mic GND ────────┤ GND         │
                    │             │
                    └─────────────┘
```

## MFRC522 Connection Details

The MFRC522 uses SPI communication:

| MFRC522 Pin | ESP8266 Pin | Description |
|-------------|-------------|-------------|
| SDA (SS) | D8 (GPIO15) | Slave Select |
| SCK | D5 (GPIO14) | SPI Clock |
| MOSI | D7 (GPIO13) | Master Out Slave In |
| MISO | D6 (GPIO12) | Master In Slave Out |
| IRQ | Not connected | Interrupt (optional) |
| GND | GND | Ground |
| RST | D3 (GPIO0) | Reset |
| 3.3V | 3.3V | Power |

⚠️ **Important**: The MFRC522 operates at 3.3V. Do NOT connect to 5V!

## NeoPixel LED Strip Connection

| LED Strip Pin | ESP8266 Pin | Description |
|---------------|-------------|-------------|
| DIN | D2 (GPIO4) | Data In |
| 5V/VCC | VIN | 5V Power |
| GND | GND | Ground |

⚠️ **Notes**:
- For strips longer than 8 LEDs, use external 5V power supply
- Add a 300-500Ω resistor between GPIO4 and DIN for signal protection
- Add a 1000µF capacitor across power rails for stability

## Sound Sensor Module (Optional)

| Sound Module Pin | ESP8266 Pin | Description |
|------------------|-------------|-------------|
| OUT/AO | A0 | Analog Output |
| VCC | 3.3V | Power |
| GND | GND | Ground |

Adjust the sensitivity potentiometer on the sound module for optimal response.

## Assembly Steps

### Step 1: Prepare Components
1. Gather all components
2. Inspect for damage
3. Test ESP8266 by uploading a blink sketch

### Step 2: Connect MFRC522
1. Wire SPI connections as shown above
2. Double-check 3.3V power connection
3. Ensure solid connections (poor connections cause read failures)

### Step 3: Connect NeoPixel Strip
1. Connect data pin to GPIO4
2. Connect power from VIN (5V from USB)
3. Share GND with ESP8266

### Step 4: Connect Sound Sensor (Optional)
1. Connect analog output to A0
2. Connect power to 3.3V
3. Adjust sensitivity after software upload

### Step 5: Test
1. Upload firmware
2. Open Serial Monitor at 115200 baud
3. Verify all components initialize correctly

## Troubleshooting Hardware Issues

### MFRC522 Not Detected
- Check all SPI connections
- Verify 3.3V power supply
- Try different GPIO pins for RST/SS
- Some clone modules may have poor quality - try a different module

### LEDs Not Working
- Check data pin connection
- Verify 5V power supply
- Ensure GND is shared
- Test with simple NeoPixel example sketch

### Sound Sensor Not Responding
- Adjust sensitivity potentiometer
- Check analog reading in Serial Monitor
- Verify 3.3V power (not 5V)

## Power Requirements

| Component | Current Draw |
|-----------|--------------|
| ESP8266 | ~80mA active |
| MFRC522 | ~50mA |
| WS2812B (per LED) | ~60mA max |
| Sound Sensor | ~5mA |

**Total (8 LEDs)**: ~620mA maximum

A good quality USB power source (500mA+) should suffice for basic operation.

## 3D Printed Case

STL files for a basic enclosure are available in `hardware/stl/`:
- `case_bottom.stl` - Main enclosure
- `case_top.stl` - Lid with NFC window
- `led_diffuser.stl` - Light diffuser for LEDs

Print settings:
- Material: PLA or PETG
- Layer height: 0.2mm
- Infill: 20%
- Supports: Yes (for top piece)
