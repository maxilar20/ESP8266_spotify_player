/**
 * @file NfcReader.h
 * @brief NFC/RFID Reader for reading Spotify URIs from tags
 *
 * Manages MFRC522 NFC reader for reading Spotify URIs from NTAG/MIFARE tags.
 * Features:
 * - Interrupt-driven card detection for real-time response
 * - Non-blocking operation
 */

#ifndef NFC_READER_H
#define NFC_READER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Config.h"

// Forward declaration for interrupt handler
class NfcReader;

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
 * @brief NFC reader state for non-blocking operation
 */
enum class NfcState
{
    IDLE,           // Waiting for card
    CARD_DETECTED,  // Card detected via interrupt
    READING,        // Reading card data
    READ_COMPLETE,  // Data read successfully
    READ_FAILED     // Read operation failed
};

/**
 * @class NfcReader
 * @brief Reads Spotify URIs from NFC tags with interrupt support
 */
class NfcReader
{
public:
    /**
     * @brief Construct a new NFC Reader object
     * @param ssPin SPI Slave Select pin
     * @param rstPin Reset pin
     * @param irqPin Interrupt pin (optional, use -1 for polling mode)
     */
    NfcReader(uint8_t ssPin, uint8_t rstPin, int8_t irqPin = -1);

    /**
     * @brief Initialize the NFC reader
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Check if a new card is present (polling mode or interrupt triggered)
     * @return true if a new card is detected
     */
    bool isNewCardPresent();

    /**
     * @brief Check if interrupt was triggered (for interrupt mode)
     * @return true if card detected via interrupt
     */
    bool isCardDetectedByInterrupt() const;

    /**
     * @brief Clear the interrupt flag
     */
    void clearInterruptFlag();

    /**
     * @brief Read Spotify URI from the current card
     * @return NfcReadResult containing success status and URI
     */
    NfcReadResult readSpotifyUri();

    /**
     * @brief Stop communication with current card
     */
    void haltCard();

    /**
     * @brief Get current NFC reader state
     * @return Current NfcState
     */
    NfcState getState() const;

    /**
     * @brief Reset the reader state to IDLE
     */
    void resetState();

    /**
     * @brief Check if using interrupt mode
     * @return true if interrupt mode is enabled
     */
    bool isInterruptMode() const;

    /**
     * @brief Static interrupt handler (called from ISR)
     */
    static void IRAM_ATTR handleInterrupt();

private:
    MFRC522 mfrc522_;
    int8_t irqPin_;
    NfcState state_;

    static constexpr uint8_t PAGE_ADDRESS = 0x06;
    static constexpr uint8_t BUFFER_SIZE = 176;
    static constexpr uint8_t READ_BUFFER_SIZE = 18;
    static constexpr uint8_t URI_START_OFFSET = 26;

    // Static pointer for interrupt handler access
    static NfcReader* instance_;
    static volatile bool cardDetectedFlag_;

    /**
     * @brief Setup interrupt for card detection
     */
    void setupInterrupt();

    /**
     * @brief Parse NFC tag data buffer into Spotify URI
     * @param dataBuffer Raw data from NFC tag
     * @return Spotify URI string
     */
    String parseTagData(byte *dataBuffer);
};

#endif // NFC_READER_H
