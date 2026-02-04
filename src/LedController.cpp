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
    , rainbowOffset_(0)
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

void LedController::showTagProcessing()
{
    setState(LedState::TAG_PROCESSING);
    animationStep_ = 0;
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

void LedController::showMusicPlaying()
{
    setState(LedState::MUSIC_PLAYING);
    rainbowOffset_ = 0;
}

void LedController::showMusicPaused()
{
    setState(LedState::MUSIC_PAUSED);
    pulseBrightness_ = 50;
    pulseDirection_ = 1;
}

void LedController::showStandby()
{
    setState(LedState::STANDBY);
    pulseBrightness_ = 10;
    pulseDirection_ = 1;
}

void LedController::showStartup()
{
    setState(LedState::STARTUP);
    animationStep_ = 0;
}

void LedController::showTokenRefresh()
{
    setState(LedState::TOKEN_REFRESH);
    pulseBrightness_ = 50;
    pulseDirection_ = 1;
}

void LedController::showSearching()
{
    setState(LedState::SEARCHING);
    animationStep_ = 0;
}

void LedController::showVolumeUp()
{
    setState(LedState::VOLUME_UP);
    animationStep_ = 0;
}

void LedController::showVolumeDown()
{
    setState(LedState::VOLUME_DOWN);
    animationStep_ = numLeds_;
}

void LedController::showSkipTrack()
{
    setState(LedState::SKIP_TRACK);
    animationStep_ = 0;
}

void LedController::showPrevTrack()
{
    setState(LedState::PREV_TRACK);
    animationStep_ = 0;
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

void LedController::animateDualSpin(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < SPIN_INTERVAL) return;
    lastAnimationTime_ = currentTime;

    pixels_.clear();

    // Create two comets spinning in opposite directions
    uint16_t pos1 = animationStep_ % numLeds_;
    uint16_t pos2 = (numLeds_ - 1 - (animationStep_ % numLeds_));

    // First comet (clockwise)
    pixels_.setPixelColor(pos1, pixels_.Color(r1, g1, b1));
    pixels_.setPixelColor((pos1 + 1) % numLeds_, pixels_.Color(r1 / 2, g1 / 2, b1 / 2));
    pixels_.setPixelColor((pos1 + numLeds_ - 1) % numLeds_, pixels_.Color(r1 / 4, g1 / 4, b1 / 4));

    // Second comet (counter-clockwise)
    pixels_.setPixelColor(pos2, pixels_.Color(r2, g2, b2));
    pixels_.setPixelColor((pos2 + numLeds_ - 1) % numLeds_, pixels_.Color(r2 / 2, g2 / 2, b2 / 2));
    pixels_.setPixelColor((pos2 + 1) % numLeds_, pixels_.Color(r2 / 4, g2 / 4, b2 / 4));

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

uint32_t LedController::colorWheel(uint8_t wheelPos)
{
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return pixels_.Color(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    if (wheelPos < 170) {
        wheelPos -= 85;
        return pixels_.Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    wheelPos -= 170;
    return pixels_.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

void LedController::animateRainbowWave()
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < RAINBOW_INTERVAL) return;
    lastAnimationTime_ = currentTime;

    for (uint16_t i = 0; i < numLeds_; i++) {
        uint8_t colorIndex = ((i * 256 / numLeds_) + rainbowOffset_) & 255;
        pixels_.setPixelColor(i, colorWheel(colorIndex));
    }
    rainbowOffset_ = (rainbowOffset_ + 3) % 256;
}

void LedController::animateBreathing(uint8_t r, uint8_t g, uint8_t b)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < BREATHING_INTERVAL) return;
    lastAnimationTime_ = currentTime;

    // Slower, smoother breathing effect
    if (pulseDirection_ == 1) {
        pulseBrightness_ += 2;
        if (pulseBrightness_ >= 150) {
            pulseBrightness_ = 150;
            pulseDirection_ = 0;
        }
    } else {
        pulseBrightness_ -= 2;
        if (pulseBrightness_ <= 10) {
            pulseBrightness_ = 10;
            pulseDirection_ = 1;
        }
    }

    uint8_t scaledR = (r * pulseBrightness_) / 255;
    uint8_t scaledG = (g * pulseBrightness_) / 255;
    uint8_t scaledB = (b * pulseBrightness_) / 255;

    for (uint16_t i = 0; i < numLeds_; i++) {
        pixels_.setPixelColor(i, pixels_.Color(scaledR, scaledG, scaledB));
    }
}

void LedController::animateRainbowSweep()
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < 80) return;
    lastAnimationTime_ = currentTime;

    if (animationStep_ < numLeds_ * 3) {  // 3 complete sweeps
        uint16_t pos = animationStep_ % numLeds_;

        // Clear previous
        pixels_.clear();

        // Light up LEDs up to current position with rainbow
        for (uint16_t i = 0; i <= pos; i++) {
            uint8_t colorIndex = (i * 256 / numLeds_) & 255;
            pixels_.setPixelColor(i, colorWheel(colorIndex));
        }
        animationStep_++;
    } else {
        // Startup complete, go to idle
        setState(LedState::IDLE);
        setColor(StatusColor::IDLE_R, StatusColor::IDLE_G, StatusColor::IDLE_B, false);
    }
}

void LedController::animateSparkle(uint8_t r, uint8_t g, uint8_t b)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < SPARKLE_INTERVAL) return;
    lastAnimationTime_ = currentTime;

    // Dim all LEDs slightly
    for (uint16_t i = 0; i < numLeds_; i++) {
        uint32_t color = pixels_.getPixelColor(i);
        uint8_t cr = (color >> 16) & 0xFF;
        uint8_t cg = (color >> 8) & 0xFF;
        uint8_t cb = color & 0xFF;
        pixels_.setPixelColor(i, pixels_.Color(cr * 0.7, cg * 0.7, cb * 0.7));
    }

    // Random sparkle on 1-2 LEDs
    uint16_t sparklePos = random(numLeds_);
    pixels_.setPixelColor(sparklePos, pixels_.Color(r, g, b));

    if (random(100) > 50) {
        sparklePos = random(numLeds_);
        pixels_.setPixelColor(sparklePos, pixels_.Color(r * 0.7, g * 0.7, b * 0.7));
    }
}

void LedController::animateExpandRing(uint8_t r, uint8_t g, uint8_t b)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < 60) return;
    lastAnimationTime_ = currentTime;

    if (animationStep_ <= numLeds_) {
        pixels_.clear();

        // Light up LEDs from center outward
        uint16_t center = numLeds_ / 2;
        for (uint16_t i = 0; i < animationStep_; i++) {
            uint8_t intensity = 255 - (i * 30);
            if (intensity < 50) intensity = 50;

            uint16_t pos1 = (center + i) % numLeds_;
            uint16_t pos2 = (center - i + numLeds_) % numLeds_;

            pixels_.setPixelColor(pos1, pixels_.Color(
                (r * intensity) / 255,
                (g * intensity) / 255,
                (b * intensity) / 255
            ));
            pixels_.setPixelColor(pos2, pixels_.Color(
                (r * intensity) / 255,
                (g * intensity) / 255,
                (b * intensity) / 255
            ));
        }
        animationStep_++;
    } else {
        setState(LedState::IDLE);
        setColor(StatusColor::IDLE_R, StatusColor::IDLE_G, StatusColor::IDLE_B, false);
    }
}

void LedController::animateContractRing(uint8_t r, uint8_t g, uint8_t b)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < 60) return;
    lastAnimationTime_ = currentTime;

    if (animationStep_ > 0) {
        pixels_.clear();

        // Light up LEDs from outside inward
        uint16_t center = numLeds_ / 2;
        for (uint16_t i = 0; i < animationStep_; i++) {
            uint8_t intensity = 50 + (i * 30);
            if (intensity > 255) intensity = 255;

            uint16_t pos1 = (center + (numLeds_ / 2) - i) % numLeds_;
            uint16_t pos2 = (center - (numLeds_ / 2) + i + numLeds_) % numLeds_;

            pixels_.setPixelColor(pos1, pixels_.Color(
                (r * intensity) / 255,
                (g * intensity) / 255,
                (b * intensity) / 255
            ));
            pixels_.setPixelColor(pos2, pixels_.Color(
                (r * intensity) / 255,
                (g * intensity) / 255,
                (b * intensity) / 255
            ));
        }
        animationStep_--;
    } else {
        setState(LedState::IDLE);
        setColor(StatusColor::IDLE_R, StatusColor::IDLE_G, StatusColor::IDLE_B, false);
    }
}

void LedController::animateChase(uint8_t r, uint8_t g, uint8_t b, bool right)
{
    unsigned long currentTime = millis();
    if (currentTime - lastAnimationTime_ < CHASE_INTERVAL) return;
    lastAnimationTime_ = currentTime;

    if (animationStep_ < numLeds_ * 2) {
        pixels_.clear();

        uint16_t pos;
        if (right) {
            pos = animationStep_ % numLeds_;
        } else {
            pos = (numLeds_ - 1 - (animationStep_ % numLeds_));
        }

        // Create a comet tail effect
        for (int t = 0; t < 4 && t <= (int)animationStep_; t++) {
            uint16_t tailPos;
            if (right) {
                tailPos = (pos - t + numLeds_) % numLeds_;
            } else {
                tailPos = (pos + t) % numLeds_;
            }

            uint8_t intensity = 255 - (t * 60);
            if (intensity < 30) intensity = 30;

            pixels_.setPixelColor(tailPos, pixels_.Color(
                (r * intensity) / 255,
                (g * intensity) / 255,
                (b * intensity) / 255
            ));
        }
        animationStep_++;
    } else {
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

        case LedState::TAG_PROCESSING:
            animateDualSpin(
                StatusColor::NFC_READING_R,
                StatusColor::NFC_READING_G,
                StatusColor::NFC_READING_B,
                StatusColor::TAG_PROCESSING_R,
                StatusColor::TAG_PROCESSING_G,
                StatusColor::TAG_PROCESSING_B
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

        case LedState::MUSIC_PLAYING:
            animateRainbowWave();
            break;

        case LedState::MUSIC_PAUSED:
            animateBreathing(
                StatusColor::PAUSED_R,
                StatusColor::PAUSED_G,
                StatusColor::PAUSED_B
            );
            break;

        case LedState::STANDBY:
            animateBreathing(
                StatusColor::STANDBY_R,
                StatusColor::STANDBY_G,
                StatusColor::STANDBY_B
            );
            break;

        case LedState::STARTUP:
            animateRainbowSweep();
            break;

        case LedState::TOKEN_REFRESH:
            animatePulse(
                StatusColor::TOKEN_REFRESH_R,
                StatusColor::TOKEN_REFRESH_G,
                StatusColor::TOKEN_REFRESH_B
            );
            break;

        case LedState::SEARCHING:
            animateSparkle(
                StatusColor::SEARCHING_R,
                StatusColor::SEARCHING_G,
                StatusColor::SEARCHING_B
            );
            break;

        case LedState::VOLUME_UP:
            animateExpandRing(
                StatusColor::VOLUME_R,
                StatusColor::VOLUME_G,
                StatusColor::VOLUME_B
            );
            break;

        case LedState::VOLUME_DOWN:
            animateContractRing(
                StatusColor::VOLUME_R,
                StatusColor::VOLUME_G,
                StatusColor::VOLUME_B
            );
            break;

        case LedState::SKIP_TRACK:
            animateChase(
                StatusColor::SKIP_R,
                StatusColor::SKIP_G,
                StatusColor::SKIP_B,
                true  // Chase right
            );
            break;

        case LedState::PREV_TRACK:
            animateChase(
                StatusColor::SKIP_R,
                StatusColor::SKIP_G,
                StatusColor::SKIP_B,
                false  // Chase left
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
