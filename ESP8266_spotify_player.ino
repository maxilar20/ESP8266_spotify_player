#include <WiFiManager.h>

#include <ESP8266WiFi.h>

// RGB Strip Import
#include <Adafruit_NeoPixel.h>
#define NUM_LEDS 8
#define LED_PIN 4
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Reactive lights setup
#define ANALOG_READ A0
int leds_h[NUM_LEDS];
int mic_in; //TODO Change variabl
long actual_time = 0;
int reactive_idx;

// RC522 SETTINGS
#include <SPI.h>
#include "MFRC522.h"
#define RST_PIN 0                 // Configurable
#define SS_PIN 15                 // Configurable
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
MFRC522::StatusCode status;       //variable to get card status
byte size = 18;
uint8_t pageAddr = 0x06;
byte const BUFFERSiZE = 176;
byte buffer_[18];
byte c1[96];

// WIFI SETTINGS
// const char *ssid = "Dorne WIFI 6";
// const char *password = "dorne1234";
unsigned long previous_time = 0;
unsigned long delay_time = 20000; // 20 seconds delay

// SPOTIFY API SETTINGS
#include "SpotifyClient.h"
String refreshToken = "AQB5niKYhkNzIQJmeHhMp2mOctkmztgILz2nwf5VT3Kc3z4u1RoP4GGr20j9eoR57x5kmt7dElD6Vsj7kt7b40DlcOa0DG7CXTZIXBS4MXnc3zkvr1YmMJNmgwKD6Atp2LM";
String clientId = "ed6b1e55817049ff92eb6048532d00d8";
String clientSecret = "cc7e29ef07254c229403d090d1da7c83";
String deviceName = "Echo en la Glasgow";
SpotifyClient spotify = SpotifyClient(clientId, clientSecret, deviceName, refreshToken);

void setup()
{
    // MIC Setup
    pinMode(ANALOG_READ, INPUT);

    // RGB Strip setup
    pixels.begin();
    pixels.clear();
    pixels.setBrightness(50);
    pixels.show();

    // Serial Setup
    Serial.begin(115200);

    // Connect Wifi
    // WiFi.begin(ssid, password);
    // Serial.print("Connecting");
    // connectWifi();

    //first parameter is name of access point, second is the password
    loadColor(122, 122, 0);
    WiFiManager wifiManager;
    wifiManager.setClass("invert"); // dark theme
    wifiManager.autoConnect("AutoConnectAP", "password");

    // Connect to Spotify
    spotify.FetchToken();
    spotify.GetDevices();
    loadColor(0, 255, 0);

    // Start the NFC reader
    SPI.begin();
    mfrc522.PCD_Init();
}

void loop()
{
    // WIFI Reconection
    unsigned long current_time = millis(); // number of milliseconds since the upload
    if ((current_time - previous_time >= delay_time))
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.print(millis());
            Serial.println("Reconnecting to WIFI network");
            WiFi.disconnect();
            WiFi.reconnect();
            previous_time = current_time;
        }
    }

    // Sound Reactive
    if (millis() - actual_time > 35)
    {
        actual_time = millis();
        mic_in = analogRead(A0) / 4;

        for (int i = 0; i < ((NUM_LEDS / 2) - 1); i++)
        {
            leds_h[i] = leds_h[i + 1];
            leds_h[NUM_LEDS - 1 - i] = leds_h[(NUM_LEDS)-i - 2];
        }
    }
    leds_h[(NUM_LEDS / 2) - 1] = mic_in;
    leds_h[NUM_LEDS / 2] = mic_in;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        reactive_idx = i + 1;
        if (reactive_idx > NUM_LEDS - 1)
            reactive_idx -= 8;
        // pixels.setPixelColorHsv(reactive_idx, map(leds_h[i], 100, 200, 0, 255), 255, 255);
    }
    pixels.show();

    // Check for new card
    if (!mfrc522.PICC_IsNewCardPresent())
        return;
    if (!mfrc522.PICC_ReadCardSerial())
        return;
    Read();
}

void Read() // Read data
{
    loadColor(0, 0, 255);

    Serial.println(F("Reading data ... "));

    //data in 4 block is read at once.
    for (int i = 0; i < 6; i++)
    {
        status = mfrc522.MIFARE_Read(pageAddr + (4 * i), buffer_, &size);
        if (status != MFRC522::STATUS_OK)
        {
            Serial.print("MIFARE_Read() failed: ");
            Serial.println(mfrc522.GetStatusCodeName(status));

            delay(100);

            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            return;
        }
        for (byte j = 0; j < 16; j++)
            c1[j + i * 16] = buffer_[j];
    }

    // Playing uri
    String context_uri = parseNFCTagData(c1);
    Serial.println(context_uri);
    spotify.PlaySpotifyUri(context_uri);
    loadColor(0, 255, 0);

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void loadColor(int r, int g, int b)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(r, g, b));
        pixels.show();
        delay(50);
    }
}

void connectWifi()
{
    int led_idx = 0;
    //Check WiFi connection status
    while (WiFi.status() != WL_CONNECTED)
    {
        pixels.clear();

        pixels.setPixelColor(led_idx, pixels.Color(122, 122, 0));
        led_idx++;
        if (led_idx >= NUM_LEDS)
        {
            led_idx = 0;
        }
        pixels.show();

        delay(300);
        Serial.print(".");
    }
    Serial.println("\n Connected");
}

String parseNFCTagData(byte *dataBuffer)
{
    String retVal = "spotify:";
    for (int i = 26; i < BUFFERSiZE; i++)
    {
        if (dataBuffer[i] == 0xFE || dataBuffer[i] == 0x00)
        {
            break;
        }
        if (dataBuffer[i] == '/')
        {
            retVal += ':';
        }
        else
        {
            retVal += (char)dataBuffer[i];
        }
    }
    Serial.print("NFC tag: ");
    Serial.println(retVal);
    return retVal;
}