/**
 * @file LedController.cpp
 * @brief Implementation of LED Strip Controller
 */

#include "LedController.h"

LedController::LedController(uint8_t pin, uint16_t numLeds)
    : pixels_(numLeds, pin, LED_TYPE)
    , numLeds_(numLeds)
    , audioLevels_(nullptr)
    , lastSampleTime_(0)
    , lastAnimationTime_(0)
    , currentState_(LedState::IDLE)
    , animationStep_(0)
    , pulseDirection_(1)
    , pulseBrightness_(50)
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

void LedController::setState(LedState state)
{
    currentState_ = state;
    animationStep_ = 0;
    pulseBrightness_ = 50;
    pulseDirection_ = 1;
}

LedState LedController::getState() const
{
    return currentState_;
}

// =========================================================================
// State-based feedback methods
// =========================================================================

void LedController::showWifiConnecting()
{
    setState(LedState::WIFI_CONNECTING);
}

void LedController::showWifiError()
{
    setState(LedState::WIFI_ERROR);
}

void LedController::showSpotifyConnecting()
{
    setState(LedState::SPOTIFY_CONNECTING);
}

void LedController::showSpotifyError()
{
    setState(LedState::SPOTIFY_ERROR);
}

void LedController::showNfcReading()
{
    setState(LedState::NFC_READING);
}

void LedController::showTagSuccess()
{
    setState(LedState::TAG_SUCCESS);
    animationStep_ = 0;
}

void LedController::showTagFailure()
{
    setState(LedState::TAG_FAILURE);
    animationStep_ = 0;
}

void LedController::showDeviceSelected()
{
    setState(LedState::DEVICE_SELECTED);
    animationStep_ = 0;
}

void LedController::showIdle()
{
    setState(LedState::IDLE);
    setColor(StatusColor::IDLE_R, StatusColor::IDLE_G, StatusColor::IDLE_B, false);
}

// =========================================================================
// Legacy methods (for compatibility)
// =========================================================================

void LedController::showConnecting()
{
    showWifiConnecting();
}

void LedController::showSuccess()
{
    showTagSuccess();
}

void LedController::showReading()
{
    showNfcReading();
}

void LedController::showError()
{
    showTagFailure();
}

// =========================================================================
// Animation helpers
// =========================================================================

void LedController::animatePulse(uint8_t r, uint8_t g, uint8_t b)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < PULSE_INTERVAL) return;
    lastAnimationTime_ = currentTime;

    // Update pulse brightness
    if (pulseDirection_ == 1) {
        pulseBrightness_ += 5;
        if (pulseBrightness_ >= 255) {
            pulseBrightness_ = 255;
            pulseDirection_ = 0;
        }
    } else {
        pulseBrightness_ -= 5;
        if (pulseBrightness_ <= 20) {
            pulseBrightness_ = 20;
            pulseDirection_ = 1;
        }
    }

    // Apply brightness scaling to colors
    uint8_t scaledR = (r * pulseBrightness_) / 255;
    uint8_t scaledG = (g * pulseBrightness_) / 255;
    uint8_t scaledB = (b * pulseBrightness_) / 255;

    for (uint16_t i = 0; i < numLeds_; i++) {
        pixels_.setPixelColor(i, pixels_.Color(scaledR, scaledG, scaledB));
    }
}

void LedController::animateBlink(uint8_t r, uint8_t g, uint8_t b, unsigned long interval)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < interval) return;
    lastAnimationTime_ = currentTime;

    animationStep_ = !animationStep_;

    if (animationStep_) {
        for (uint16_t i = 0; i < numLeds_; i++) {
            pixels_.setPixelColor(i, pixels_.Color(r, g, b));
        }
    } else {
        pixels_.clear();
    }
}

void LedController::animateSpin(uint8_t r, uint8_t g, uint8_t b)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < SPIN_INTERVAL) return;
    lastAnimationTime_ = currentTime;

    pixels_.clear();

    // Light up 2-3 consecutive LEDs for a comet effect
    uint16_t pos = animationStep_ % numLeds_;
    pixels_.setPixelColor(pos, pixels_.Color(r, g, b));
    pixels_.setPixelColor((pos + 1) % numLeds_, pixels_.Color(r / 2, g / 2, b / 2));
    pixels_.setPixelColor((pos + numLeds_ - 1) % numLeds_, pixels_.Color(r / 4, g / 4, b / 4));

    animationStep_++;
}

void LedController::animateFlash(uint8_t r, uint8_t g, uint8_t b, uint8_t flashCount)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < 150) return;
    lastAnimationTime_ = currentTime;

    uint8_t totalSteps = flashCount * 2;

    if (animationStep_ < totalSteps) {
        if (animationStep_ % 2 == 0) {
            for (uint16_t i = 0; i < numLeds_; i++) {
                pixels_.setPixelColor(i, pixels_.Color(r, g, b));
            }
        } else {
            pixels_.clear();
        }
        animationStep_++;
    } else {
        // Flash complete, return to idle
        setState(LedState::IDLE);
        setColor(StatusColor::IDLE_R, StatusColor::IDLE_G, StatusColor::IDLE_B, false);
    }
}

void LedController::updateAnimation()
{
    switch (currentState_) {
        case LedState::WIFI_CONNECTING:
            animatePulse(
                StatusColor::WIFI_CONNECTING_R,
                StatusColor::WIFI_CONNECTING_G,
                StatusColor::WIFI_CONNECTING_B
            );
            break;

        case LedState::WIFI_ERROR:
            animateBlink(
                StatusColor::WIFI_ERROR_R,
                StatusColor::WIFI_ERROR_G,
                StatusColor::WIFI_ERROR_B,
                BLINK_FAST_INTERVAL
            );
            break;

        case LedState::SPOTIFY_CONNECTING:
            animatePulse(
                StatusColor::SPOTIFY_CONNECTING_R,
                StatusColor::SPOTIFY_CONNECTING_G,
                StatusColor::SPOTIFY_CONNECTING_B
            );
            break;

        case LedState::SPOTIFY_ERROR:
            animateBlink(
                StatusColor::SPOTIFY_ERROR_R,
                StatusColor::SPOTIFY_ERROR_G,
                StatusColor::SPOTIFY_ERROR_B,
                BLINK_SLOW_INTERVAL
            );
            break;

        case LedState::NFC_READING:
            animateSpin(
                StatusColor::NFC_READING_R,
                StatusColor::NFC_READING_G,
                StatusColor::NFC_READING_B
            );
            break;

        case LedState::TAG_SUCCESS:
            animateFlash(
                StatusColor::SUCCESS_R,
                StatusColor::SUCCESS_G,
                StatusColor::SUCCESS_B,
                3
            );
            break;

        case LedState::TAG_FAILURE:
            animateFlash(
                StatusColor::WIFI_ERROR_R,
                StatusColor::WIFI_ERROR_G,
                StatusColor::WIFI_ERROR_B,
                5
            );
            break;

        case LedState::DEVICE_SELECTED:
            animateFlash(
                StatusColor::DEVICE_SELECTED_R,
                StatusColor::DEVICE_SELECTED_G,
                StatusColor::DEVICE_SELECTED_B,
                2
            );
            break;

        case LedState::IDLE:
        default:
            // Idle state - handled by sound reactive or stays dim green
            break;
    }
}

void LedController::updateSoundReactive(int audioLevel)
{
    // Only do sound reactive when in IDLE state
    if (currentState_ != LedState::IDLE) {
        return;
    }

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

    // Update LED colors based on audio levels (green with intensity)
    for (uint16_t i = 0; i < numLeds_; i++) {
        uint8_t intensity = constrain(audioLevels_[i], 20, 255);
        pixels_.setPixelColor(i, pixels_.Color(0, intensity, 0));
    }
}

void LedController::update()
{
    // Run animations for non-idle states
    if (currentState_ != LedState::IDLE) {
        updateAnimation();
    }

    pixels_.show();
}
