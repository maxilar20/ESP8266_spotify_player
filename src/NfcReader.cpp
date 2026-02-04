/**
 * @file NfcReader.cpp
 * @brief Implementation of NFC Reader for Spotify tags
 */

#include "NfcReader.h"

NfcReader::NfcReader(uint8_t ssPin, uint8_t rstPin)
    : mfrc522_(ssPin, rstPin)
{
}

bool NfcReader::begin() {
    SPI.begin();
    mfrc522_.PCD_Init();

    // Check if reader is responding
    byte version = mfrc522_.PCD_ReadRegister(MFRC522::VersionReg);
    if (version == 0x00 || version == 0xFF) {
        DEBUG_PRINTLN(F("NFC reader not detected"));
        return false;
    }

    DEBUG_PRINT(F("NFC reader initialized. Version: 0x"));
    DEBUG_PRINTLN(String(version, HEX));
    return true;
}

bool NfcReader::isNewCardPresent() {
    if (!mfrc522_.PICC_IsNewCardPresent()) {
        return false;
    }
    return mfrc522_.PICC_ReadCardSerial();
}

NfcReadResult NfcReader::readSpotifyUri() {
    NfcReadResult result;
    result.success = false;

    DEBUG_PRINTLN(F("Reading NFC tag data..."));

    byte buffer[READ_BUFFER_SIZE];
    byte bufferSize = READ_BUFFER_SIZE;
    byte fullData[BUFFER_SIZE];

    // Read data from multiple pages (4 bytes per page, read 16 bytes at a time)
    for (int i = 0; i < 6; i++) {
        MFRC522::StatusCode status = mfrc522_.MIFARE_Read(
            PAGE_ADDRESS + (4 * i),
            buffer,
            &bufferSize
        );

        if (status != MFRC522::STATUS_OK) {
            result.errorMessage = F("Read failed: ");
            result.errorMessage += mfrc522_.GetStatusCodeName(status);
            DEBUG_PRINTLN(result.errorMessage);

            haltCard();
            return result;
        }

        // Copy to full data buffer
        for (byte j = 0; j < 16; j++) {
            fullData[j + i * 16] = buffer[j];
        }
    }

    // Parse the URI from the data
    result.spotifyUri = parseTagData(fullData);
    result.success = !result.spotifyUri.isEmpty();

    if (!result.success) {
        result.errorMessage = F("No valid Spotify URI found on tag");
    }

    DEBUG_PRINT(F("Parsed URI: "));
    DEBUG_PRINTLN(result.spotifyUri);

    haltCard();
    return result;
}

void NfcReader::haltCard() {
    mfrc522_.PICC_HaltA();
    mfrc522_.PCD_StopCrypto1();
}

String NfcReader::parseTagData(byte* dataBuffer) {
    String uri = "spotify:";

    for (int i = URI_START_OFFSET; i < BUFFER_SIZE; i++) {
        byte currentByte = dataBuffer[i];

        // End markers
        if (currentByte == 0xFE || currentByte == 0x00) {
            break;
        }

        // Convert URL path separator to Spotify URI separator
        if (currentByte == '/') {
            uri += ':';
        } else {
            uri += static_cast<char>(currentByte);
        }
    }

    return uri;
}
