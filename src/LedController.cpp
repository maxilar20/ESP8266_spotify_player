/**
 * @file LedController.cpp
 * @brief Implementation of LED Strip Controller
 */

#include "LedController.h"

LedController::LedController(uint8_t pin, uint16_t numLeds)
    : pixels_(numLeds, pin, LED_TYPE), numLeds_(numLeds), audioLevels_(nullptr), lastSampleTime_(0)
{
    audioLevels_ = new int[numLeds_]();
}

void LedController::begin()
{
    pixels_.begin();
    pixels_.clear();
    pixels_.setBrightness(LED_BRIGHTNESS);
    pixels_.show();
}

void LedController::clear()
{
    pixels_.clear();
    pixels_.show();
}

void LedController::setBrightness(uint8_t brightness)
{
    pixels_.setBrightness(brightness);
    pixels_.show();
}

void LedController::setColor(uint8_t r, uint8_t g, uint8_t b, bool animate)
{
    for (uint16_t i = 0; i < numLeds_; i++)
    {
        pixels_.setPixelColor(i, pixels_.Color(r, g, b));
        if (animate)
        {
            pixels_.show();
            delay(ANIMATION_DELAY);
        }
    }
    if (!animate)
    {
        pixels_.show();
    }
}

void LedController::showConnecting()
{
    setColor(
        StatusColor::CONNECTING_R,
        StatusColor::CONNECTING_G,
        StatusColor::CONNECTING_B);
}

void LedController::showSuccess()
{
    setColor(
        StatusColor::SUCCESS_R,
        StatusColor::SUCCESS_G,
        StatusColor::SUCCESS_B);
}

void LedController::showReading()
{
    setColor(
        StatusColor::READING_R,
        StatusColor::READING_G,
        StatusColor::READING_B);
}

void LedController::showError()
{
    setColor(
        StatusColor::ERROR_R,
        StatusColor::ERROR_G,
        StatusColor::ERROR_B);
}

void LedController::updateSoundReactive(int audioLevel)
{
    unsigned long currentTime = millis();

    if (currentTime - lastSampleTime_ < MIC_SAMPLE_INTERVAL)
    {
        return;
    }

    lastSampleTime_ = currentTime;

    // Scale audio level
    int scaledLevel = audioLevel / 4;

    // Shift audio levels for visualization effect
    uint16_t halfLeds = numLeds_ / 2;

    for (uint16_t i = 0; i < halfLeds - 1; i++)
    {
        audioLevels_[i] = audioLevels_[i + 1];
        audioLevels_[numLeds_ - 1 - i] = audioLevels_[numLeds_ - i - 2];
    }

    // Set center LEDs to current audio level
    audioLevels_[halfLeds - 1] = scaledLevel;
    audioLevels_[halfLeds] = scaledLevel;
}

void LedController::update()
{
    pixels_.show();
}
