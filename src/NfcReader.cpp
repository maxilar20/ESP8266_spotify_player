/**
 * @file NfcReader.cpp
 * @brief Implementation of NFC Reader for Spotify tags
 *
 * Features:
 * - Interrupt-driven card detection
 * - Non-blocking state machine
 */

#include "NfcReader.h"

// Static members initialization
NfcReader* NfcReader::instance_ = nullptr;
volatile bool NfcReader::cardDetectedFlag_ = false;

NfcReader::NfcReader(uint8_t ssPin, uint8_t rstPin, int8_t irqPin)
    : mfrc522_(ssPin, rstPin)
    , irqPin_(irqPin)
    , state_(NfcState::IDLE)
{
    instance_ = this;
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

    // Setup interrupt if pin is configured
    if (irqPin_ >= 0) {
        setupInterrupt();
        DEBUG_PRINTLN(F("NFC interrupt mode enabled"));
    } else {
        DEBUG_PRINTLN(F("NFC polling mode enabled"));
    }

    return true;
}

void NfcReader::setupInterrupt() {
    if (irqPin_ < 0) return;

    pinMode(irqPin_, INPUT_PULLUP);

    // Clear any pending interrupts on the MFRC522
    mfrc522_.PCD_WriteRegister(MFRC522::ComIrqReg, 0x7F);

    // Enable interrupt on card detection
    // Set IRQ pin to signal when a card is detected
    mfrc522_.PCD_WriteRegister(MFRC522::DivIEnReg, 0x90);  // Enable IRQ for card detection

    // Attach interrupt handler
    attachInterrupt(digitalPinToInterrupt(irqPin_), handleInterrupt, FALLING);

    // Start the antenna and enable receiving
    mfrc522_.PCD_AntennaOn();
}

void IRAM_ATTR NfcReader::handleInterrupt() {
    cardDetectedFlag_ = true;
    if (instance_) {
        instance_->state_ = NfcState::CARD_DETECTED;
    }
}

bool NfcReader::isCardDetectedByInterrupt() const {
    return cardDetectedFlag_;
}

void NfcReader::clearInterruptFlag() {
    cardDetectedFlag_ = false;
    if (irqPin_ >= 0) {
        // Clear interrupt flags on the MFRC522
        mfrc522_.PCD_WriteRegister(MFRC522::ComIrqReg, 0x7F);
    }
}

NfcState NfcReader::getState() const {
    return state_;
}

void NfcReader::resetState() {
    state_ = NfcState::IDLE;
    clearInterruptFlag();
}

bool NfcReader::isInterruptMode() const {
    return irqPin_ >= 0;
}

bool NfcReader::isNewCardPresent() {
    // Check interrupt flag first if in interrupt mode
    if (irqPin_ >= 0 && cardDetectedFlag_) {
        // Verify card is actually present
        if (mfrc522_.PICC_IsNewCardPresent() && mfrc522_.PICC_ReadCardSerial()) {
            state_ = NfcState::CARD_DETECTED;
            return true;
        }
        // False positive, clear flag
        clearInterruptFlag();
        return false;
    }

    // Polling mode fallback
    if (!mfrc522_.PICC_IsNewCardPresent()) {
        return false;
    }
    if (!mfrc522_.PICC_ReadCardSerial()) {
        return false;
    }

    state_ = NfcState::CARD_DETECTED;
    return true;
}

NfcReadResult NfcReader::readSpotifyUri() {
    NfcReadResult result;
    result.success = false;

    state_ = NfcState::READING;
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

            state_ = NfcState::READ_FAILED;
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
        state_ = NfcState::READ_FAILED;
    } else {
        state_ = NfcState::READ_COMPLETE;
    }

    DEBUG_PRINT(F("Parsed URI: "));
    DEBUG_PRINTLN(result.spotifyUri);

    haltCard();
    return result;
}

void NfcReader::haltCard() {
    mfrc522_.PICC_HaltA();
    mfrc522_.PCD_StopCrypto1();
    clearInterruptFlag();
    state_ = NfcState::IDLE;
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
