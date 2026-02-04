/**
 * @file NfcReader.h
 * @brief NFC/RFID Reader for reading Spotify URIs from tags
 *
 * Manages MFRC522 NFC reader for reading Spotify URIs from NTAG/MIFARE tags.
 */

#ifndef NFC_READER_H
#define NFC_READER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Config.h"

/**
 * @brief Result of an NFC read operation
 */
struct NfcReadResult
{
    bool success;
    String spotifyUri;
    String errorMessage;
};

/**
 * @class NfcReader
 * @brief Reads Spotify URIs from NFC tags
 */
class NfcReader
{
public:
    /**
     * @brief Construct a new NFC Reader object
     * @param ssPin SPI Slave Select pin
     * @param rstPin Reset pin
     */
    NfcReader(uint8_t ssPin, uint8_t rstPin);

    /**
     * @brief Initialize the NFC reader
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Check if a new card is present
     * @return true if a new card is detected
     */
    bool isNewCardPresent();

    /**
     * @brief Read Spotify URI from the current card
     * @return NfcReadResult containing success status and URI
     */
    NfcReadResult readSpotifyUri();

    /**
     * @brief Stop communication with current card
     */
    void haltCard();

private:
    MFRC522 mfrc522_;

    static constexpr uint8_t PAGE_ADDRESS = 0x06;
    static constexpr uint8_t BUFFER_SIZE = 176;
    static constexpr uint8_t READ_BUFFER_SIZE = 18;
    static constexpr uint8_t URI_START_OFFSET = 26;

    /**
     * @brief Parse NFC tag data buffer into Spotify URI
     * @param dataBuffer Raw data from NFC tag
     * @return Spotify URI string
     */
    String parseTagData(byte *dataBuffer);
};

#endif // NFC_READER_H
