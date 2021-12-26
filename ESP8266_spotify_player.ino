#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <base64.h>
#include <WiFiClient.h>

#include <Adafruit_NeoPixel.h>
#define NUM_LEDS 8
#define PIN 4
Adafruit_NeoPixel pixels(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

//The pin that we read sensor values form
#define ANALOG_READ A0
int leds_h[NUM_LEDS];
int s;
long now = 0;
int reactive_idx;

// RC522 SETTINGS
#include <SPI.h>
#include "MFRC522.h"
#define RST_PIN 0                 // Configurable
#define SS_PIN 15                 // Configurable
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
MFRC522::StatusCode status;       //variable to get card status
byte size = 18;
uint8_t pageAddr = 0x06; //In this example we will write/read 16 bytes (page 6,7,8 and 9).
                         //Ultraligth mem = 16 pages. 4 bytes per page.
                         //Pages 0 to 4 are for special functions.
byte const BUFFERSiZE = 176;
byte buffer_[18];
byte c1[96];

// WIFI SETTINGS
WiFiClientSecure wifiClient;
const char *ssid = "Dorne WIFI 6";
const char *password = "dorne1234";
unsigned long previous_time = 0;
unsigned long delay = 20000; // 20 seconds delay

// SPOTIFY API SETTINGS
String refreshToken = "AQB5niKYhkNzIQJmeHhMp2mOctkmztgILz2nwf5VT3Kc3z4u1RoP4GGr20j9eoR57x5kmt7dElD6Vsj7kt7b40DlcOa0DG7CXTZIXBS4MXnc3zkvr1YmMJNmgwKD6Atp2LM";
String clientId = "ed6b1e55817049ff92eb6048532d00d8";
String clientSecret = "cc7e29ef07254c229403d090d1da7c83";
String deviceName = "edabc07b-f364-442b-8123-23ab3bbc798c";
String redirectUri;
String deviceId;
String accessToken;

struct HttpResult
{
    int httpCode;
    String payload;
};

void setup()
{

    pinMode(ANALOG_READ, INPUT);

    pixels.begin();
    pixels.clear();
    pixels.setBrightness(50);
    pixels.show();

    // Serial Setup
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    Serial.print("Connecting");

    int led_idx = 0;
    while (WiFi.status() != WL_CONNECTED) //Check WiFi connection status
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

    for (int i = 0; i < NUM_LEDS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(122, 122, 0));
        pixels.show();
        delay(50);
    }

    if (WiFi.status() == WL_CONNECTED) //Check WiFi connection status
    {
        FetchToken();
        Serial.print("Token:");
        Serial.print(accessToken);
        GetDevices();
    }

    for (int i = 0; i < NUM_LEDS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
        pixels.show();
        delay(50);
    }

    // Init SPI bus and MFRC522 for NFC reader
    SPI.begin();
    mfrc522.PCD_Init();

    delay(5);
}

void loop()
{
    // WIFI Reconection
    unsigned long current_time = millis(); // number of milliseconds since the upload
    if ((current_time - previous_time >= delay))
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
    if (millis() - now > 35)
    {
        now = millis();
        s = analogRead(A0) / 4;
        // Serial.println(s);

        for (int i = 0; i < ((NUM_LEDS / 2) - 1); i++)
        {
            leds_h[i] = leds_h[i + 1];
            leds_h[NUM_LEDS - 1 - i] = leds_h[(NUM_LEDS)-i - 2];
        }
    }
    leds_h[(NUM_LEDS / 2) - 1] = s;
    leds_h[NUM_LEDS / 2] = s;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        reactive_idx = i + 1;
        if (reactive_idx > NUM_LEDS - 1)
            reactive_idx -= 8;
        pixels.setPixelColorHsv(reactive_idx, map(leds_h[i], 100, 200, 0, 255), 255, 255);
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
    for (int i = 0; i < NUM_LEDS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(0, 0, 255));
        pixels.show();
        delay(50);
    }

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
    playSpotifyUri(context_uri);

    for (int i = 0; i < NUM_LEDS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
        pixels.show();
        delay(50);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void FetchToken()
{
    HTTPClient http;
    wifiClient.setInsecure(); //the magic line, use with caution

    String body = "grant_type=refresh_token&refresh_token=" + refreshToken;
    String authorizationRaw = clientId + ":" + clientSecret;
    String authorization = base64::encode(authorizationRaw);
    http.begin(wifiClient, "https://accounts.spotify.com/api/token");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Basic " + authorization);

    int httpCode = http.POST(body);
    if (httpCode > 0)
    {
        String returnedPayload = http.getString();
        if (httpCode == 200)
        {
            accessToken = ParseJson("access_token", returnedPayload);
            Serial.println("Got new access token");
        }
        else
        {
            Serial.println("Failed to get new access token");
            Serial.println(httpCode);
            Serial.println(returnedPayload);
        }
    }
    else
    {
        Serial.println("Failed to connect to https://accounts.spotify.com/api/token");
        Serial.println(httpCode);
        String returnedPayload = http.getString();
        Serial.println(returnedPayload);
    }
    http.end();
}

void GetDevices()
{
    HttpResult result = CallAPI("GET", "https://api.spotify.com/v1/me/player/devices", "");
    Serial.println(result.payload);
    deviceId = GetDeviceId(result.payload);
    Serial.print("Device ID: ");
    Serial.println(deviceId);
}

String GetDeviceId(String json)
{
    String id = "";

    // find the position the device name
    int index = json.indexOf(deviceName);
    if (index > 0)
    {
        // we found it, so not backup to begining of this object
        int i = index;
        for (; i > 0; i--)
        {
            if (json.charAt(i) == '{')
            {
                break;
            }
        }

        // now move forward and find "id"
        for (; i < json.length(); i++)
        {
            if (json.charAt(i) == '}')
            {
                break;
            }

            if (i + 3 < json.length() && json.charAt(i) == '"' && json.charAt(i + 1) == 'i' && json.charAt(i + 2) == 'd' && json.charAt(i + 3) == '"')
            {
                i += 4;
                break;
            }
        }

        // move forward to next "
        for (; i < json.length(); i++)
        {
            if (json.charAt(i) == '"')
            {
                i++;
                break;
            }
        }

        // now get that device id
        for (; i < json.length(); i++)
        {
            if (json.charAt(i) != '"')
            {
                id += json.charAt(i);
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        Serial.print(deviceName);
        Serial.println(" device name not found.");
    }
    return id;
}

int Play(String context_uri)
{
    Serial.println("Play()");
    String body = "{\"context_uri\":\"" + context_uri + "\",\"offset\":{\"position\":0,\"position_ms\":0}}";
    Serial.print("body");
    Serial.println(body);
    String url = "https://api.spotify.com/v1/me/player/play?device_id=" + deviceId;

    HttpResult result = CallAPI("PUT", url, body);
    Serial.println(result.payload);
    return result.httpCode;
}

int Next()
{
    Serial.println("Next()");
    HttpResult result = CallAPI("POST", "https://api.spotify.com/v1/me/player/next?device_id=" + deviceId, "");
    Serial.print("next:");
    Serial.println(result.payload);
    return result.httpCode;
}

int Shuffle()
{
    Serial.println("Shuffle()");
    HttpResult result = CallAPI("PUT", "https://api.spotify.com/v1/me/player/shuffle?state=true&device_id=" + deviceId, "");
    return result.httpCode;
}

void playSpotifyUri(String context_uri)
{
    int code = Play(context_uri);
    switch (code)
    {
    case 404:
    {
        // device id changed, get new one
        GetDevices();
        Play(context_uri);
        break;
    }
    case 401:
    {
        // auth token expired, get new one
        FetchToken();
        Play(context_uri);
        break;
    }
    default:
    {
        break;
    }
    }
}

HttpResult CallAPI(String method, String url, String body)
{
    HttpResult result;
    result.httpCode = 0;
    Serial.print(url);
    Serial.print(" returned: ");

    HTTPClient http;

    http.begin(wifiClient, url);

    String authorization = "Bearer " + accessToken;

    http.addHeader(F("Content-Type"), "application/json");
    http.addHeader(F("Authorization"), authorization);

    // address bug where Content-Length not added by HTTPClient is content length is zero
    if (body.length() == 0)
    {
        http.addHeader(F("Content-Length"), String(0));
    }

    if (method == "PUT")
    {
        result.httpCode = http.PUT(body);
    }
    else if (method == "POST")
    {
        result.httpCode = http.POST(body);
    }
    else if (method == "GET")
    {
        result.httpCode = http.GET();
    }

    if (result.httpCode > 0)
    {
        Serial.println(result.httpCode);
        if (http.getSize() > 0)
        {
            result.payload = http.getString();
        }
    }
    else
    {
        Serial.print("Failed to connect to ");
        Serial.println(url);
    }
    http.end();

    return result;
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

String ParseJson(String key, String json)
{
    String retVal = "";
    int index = json.indexOf(key);

    if (index > 0)
    {
        bool copy = false;
        for (int i = index; i < json.length(); i++)
        {
            //Serial.print( "charAt: " );
            //Serial.println( json.charAt(i) );
            if (copy)
            {
                if (json.charAt(i) == '"' || json.charAt(i) == ',')
                {
                    break;
                }
                else
                {
                    retVal += json.charAt(i);
                }
            }
            else if (json.charAt(i) == ':')
            {
                copy = true;
                if (json.charAt(i + 1) == '"')
                {
                    i++;
                }
            }
        }
    }
    return retVal;
}
