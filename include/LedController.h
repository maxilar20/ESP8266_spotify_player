/**
 * @file LedController.h
 * @brief LED Strip Controller for visual feedback
 *
 * Manages NeoPixel LED strip for status indication and sound reactive effects.
 */

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"

/**
 * @brief LED status states for different feedback scenarios
 */
enum class LedState {
    IDLE,               // Default state - green, sound reactive
    WIFI_CONNECTING,    // Yellow pulsing - connecting to WiFi
    WIFI_ERROR,         // Red blinking fast - no WiFi connection
    SPOTIFY_CONNECTING, // Blue pulsing - connecting to Spotify
    SPOTIFY_ERROR,      // Orange blinking - Spotify auth/device error
    NFC_READING,        // Blue spinning - reading NFC tag
    TAG_PROCESSING,     // Blue-purple alternating spin - processing/sending to Spotify
    TAG_SUCCESS,        // Green flash then idle - tag played successfully
    TAG_FAILURE,        // Red flash then idle - tag failed to play
    DEVICE_SELECTED,    // Cyan flash - device selected from web UI
    MUSIC_PLAYING,      // Rainbow wave - music is playing
    MUSIC_PAUSED,       // Dim blue breathing - playback paused
    STANDBY,            // Very dim white breathing - low power standby
    STARTUP,            // Rainbow sweep - device booting
    TOKEN_REFRESH,      // Purple pulse - refreshing Spotify token
    SEARCHING,          // White sparkle - searching for devices
    VOLUME_UP,          // Green expanding ring - volume increased
    VOLUME_DOWN,        // Green contracting ring - volume decreased
    SKIP_TRACK,         // Cyan chase right - skipping to next
    PREV_TRACK          // Cyan chase left - going to previous
};

/**
 * @brief Predefined status colors for visual feedback
 */
namespace StatusColor
{
    // WiFi connecting - yellow
    constexpr uint8_t WIFI_CONNECTING_R = 255;
    constexpr uint8_t WIFI_CONNECTING_G = 200;
    constexpr uint8_t WIFI_CONNECTING_B = 0;

    // WiFi error - red
    constexpr uint8_t WIFI_ERROR_R = 255;
    constexpr uint8_t WIFI_ERROR_G = 0;
    constexpr uint8_t WIFI_ERROR_B = 0;

    // Spotify connecting - blue
    constexpr uint8_t SPOTIFY_CONNECTING_R = 0;
    constexpr uint8_t SPOTIFY_CONNECTING_G = 100;
    constexpr uint8_t SPOTIFY_CONNECTING_B = 255;

    // Spotify error - orange
    constexpr uint8_t SPOTIFY_ERROR_R = 255;
    constexpr uint8_t SPOTIFY_ERROR_G = 80;
    constexpr uint8_t SPOTIFY_ERROR_B = 0;

    // NFC reading - blue spinning
    constexpr uint8_t NFC_READING_R = 0;
    constexpr uint8_t NFC_READING_G = 0;
    constexpr uint8_t NFC_READING_B = 255;

    // Tag processing - purple/violet (mix of blue and magenta)
    constexpr uint8_t TAG_PROCESSING_R = 138;
    constexpr uint8_t TAG_PROCESSING_G = 43;
    constexpr uint8_t TAG_PROCESSING_B = 226;

    // Success - green
    constexpr uint8_t SUCCESS_R = 0;
    constexpr uint8_t SUCCESS_G = 255;
    constexpr uint8_t SUCCESS_B = 0;

    // Idle - dim green
    constexpr uint8_t IDLE_R = 0;
    constexpr uint8_t IDLE_G = 100;
    constexpr uint8_t IDLE_B = 0;

    // Device selected - cyan
    constexpr uint8_t DEVICE_SELECTED_R = 0;
    constexpr uint8_t DEVICE_SELECTED_G = 255;
    constexpr uint8_t DEVICE_SELECTED_B = 255;

    // Music playing - uses rainbow (no static color)
    // Music paused - dim blue
    constexpr uint8_t PAUSED_R = 30;
    constexpr uint8_t PAUSED_G = 50;
    constexpr uint8_t PAUSED_B = 100;

    // Standby - very dim white
    constexpr uint8_t STANDBY_R = 20;
    constexpr uint8_t STANDBY_G = 20;
    constexpr uint8_t STANDBY_B = 20;

    // Token refresh - purple
    constexpr uint8_t TOKEN_REFRESH_R = 150;
    constexpr uint8_t TOKEN_REFRESH_G = 0;
    constexpr uint8_t TOKEN_REFRESH_B = 200;

    // Searching - white
    constexpr uint8_t SEARCHING_R = 200;
    constexpr uint8_t SEARCHING_G = 200;
    constexpr uint8_t SEARCHING_B = 200;

    // Volume/Skip - cyan/green
    constexpr uint8_t VOLUME_R = 0;
    constexpr uint8_t VOLUME_G = 200;
    constexpr uint8_t VOLUME_B = 100;

    constexpr uint8_t SKIP_R = 0;
    constexpr uint8_t SKIP_G = 200;
    constexpr uint8_t SKIP_B = 255;

    // Legacy colors for compatibility
    constexpr uint8_t CONNECTING_R = 122;
    constexpr uint8_t CONNECTING_G = 122;
    constexpr uint8_t CONNECTING_B = 0;

    constexpr uint8_t READING_R = 0;
    constexpr uint8_t READING_G = 0;
    constexpr uint8_t READING_B = 255;

    constexpr uint8_t ERROR_R = 255;
    constexpr uint8_t ERROR_G = 0;
    constexpr uint8_t ERROR_B = 0;
}

/**
 * @class LedController
 * @brief Controls NeoPixel LED strip for status and effects
 */
class LedController
{
public:
    /**
     * @brief Construct a new Led Controller object
     * @param pin GPIO pin connected to LED data line
     * @param numLeds Number of LEDs in the strip
     */
    LedController(uint8_t pin, uint16_t numLeds);

    /**
     * @brief Initialize the LED strip
     */
    void begin();

    /**
     * @brief Clear all LEDs
     */
    void clear();

    /**
     * @brief Set brightness level
     * @param brightness Brightness value (0-255)
     */
    void setBrightness(uint8_t brightness);

    /**
     * @brief Set all LEDs to a solid color with animation
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     * @param animate Enable sequential animation
     */
    void setColor(uint8_t r, uint8_t g, uint8_t b, bool animate = true);

    /**
     * @brief Set the current LED state
     * @param state The new state to display
     */
    void setState(LedState state);

    /**
     * @brief Get the current LED state
     * @return Current LedState
     */
    LedState getState() const;

    // =========================================================================
    // State-based feedback methods (new)
    // =========================================================================

    /**
     * @brief Show WiFi connecting animation (yellow pulsing)
     */
    void showWifiConnecting();

    /**
     * @brief Show WiFi error (red fast blink)
     */
    void showWifiError();

    /**
     * @brief Show Spotify connecting animation (blue pulsing)
     */
    void showSpotifyConnecting();

    /**
     * @brief Show Spotify error (orange blink)
     */
    void showSpotifyError();

    /**
     * @brief Show NFC reading animation (blue spinning)
     */
    void showNfcReading();

    /**
     * @brief Show tag processing animation (purple alternating spin)
     */
    void showTagProcessing();

    /**
     * @brief Show tag success (green flash then idle)
     */
    void showTagSuccess();

    /**
     * @brief Show tag failure (red flash then idle)
     */
    void showTagFailure();

    /**
     * @brief Show device selected (cyan flash)
     */
    void showDeviceSelected();

    /**
     * @brief Show idle state (dim green, sound reactive enabled)
     */
    void showIdle();

    /**
     * @brief Show music playing animation (rainbow wave)
     */
    void showMusicPlaying();

    /**
     * @brief Show music paused state (dim blue breathing)
     */
    void showMusicPaused();

    /**
     * @brief Show standby mode (very dim white breathing)
     */
    void showStandby();

    /**
     * @brief Show startup animation (rainbow sweep)
     */
    void showStartup();

    /**
     * @brief Show token refresh (purple pulse)
     */
    void showTokenRefresh();

    /**
     * @brief Show device searching (white sparkle)
     */
    void showSearching();

    /**
     * @brief Show volume up feedback (expanding green ring)
     */
    void showVolumeUp();

    /**
     * @brief Show volume down feedback (contracting green ring)
     */
    void showVolumeDown();

    /**
     * @brief Show skip to next track (cyan chase right)
     */
    void showSkipTrack();

    /**
     * @brief Show previous track (cyan chase left)
     */
    void showPrevTrack();

    // =========================================================================
    // Legacy methods (kept for compatibility)
    // =========================================================================

    /**
     * @brief Show connecting status animation
     */
    void showConnecting();

    /**
     * @brief Show success status
     */
    void showSuccess();

    /**
     * @brief Show reading/processing status
     */
    void showReading();

    /**
     * @brief Show error status
     */
    void showError();

    /**
     * @brief Update sound reactive visualization
     * @param audioLevel Current audio level (0-1023)
     */
    void updateSoundReactive(int audioLevel);

    /**
     * @brief Update LED display (call in main loop)
     */
    void update();

private:
    Adafruit_NeoPixel pixels_;
    uint16_t numLeds_;
    int *audioLevels_;
    unsigned long lastSampleTime_;
    unsigned long lastAnimationTime_;
    LedState currentState_;
    uint8_t animationStep_;
    uint8_t pulseDirection_; // 0 = dimming, 1 = brightening
    uint8_t pulseBrightness_;

    /**
     * @brief Update animation based on current state
     */
    void updateAnimation();

    /**
     * @brief Pulse animation helper
     */
    void animatePulse(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Blink animation helper
     */
    void animateBlink(uint8_t r, uint8_t g, uint8_t b, unsigned long interval);

    /**
     * @brief Spinning animation helper
     */
    void animateSpin(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Dual spinning animation with alternating colors
     */
    void animateDualSpin(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2);

    /**
     * @brief Flash animation then return to idle
     */
    void animateFlash(uint8_t r, uint8_t g, uint8_t b, uint8_t flashCount);

    /**
     * @brief Rainbow wave animation for music playing
     */
    void animateRainbowWave();

    /**
     * @brief Slow breathing animation
     */
    void animateBreathing(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Rainbow sweep animation for startup
     */
    void animateRainbowSweep();

    /**
     * @brief Sparkle effect for searching
     */
    void animateSparkle(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Expanding ring effect
     */
    void animateExpandRing(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Contracting ring effect
     */
    void animateContractRing(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Chase animation in specified direction
     * @param right True for clockwise, false for counter-clockwise
     */
    void animateChase(uint8_t r, uint8_t g, uint8_t b, bool right);

    /**
     * @brief Color wheel helper for rainbow effects
     * @param wheelPos Position on color wheel (0-255)
     * @return 32-bit color value
     */
    uint32_t colorWheel(uint8_t wheelPos);

    uint16_t rainbowOffset_;  // Offset for rainbow animation

    static constexpr uint8_t ANIMATION_DELAY = 50;
    static constexpr unsigned long PULSE_INTERVAL = 30;
    static constexpr unsigned long BLINK_FAST_INTERVAL = 150;
    static constexpr unsigned long BLINK_SLOW_INTERVAL = 500;
    static constexpr unsigned long SPIN_INTERVAL = 100;
    static constexpr unsigned long RAINBOW_INTERVAL = 50;
    static constexpr unsigned long BREATHING_INTERVAL = 40;
    static constexpr unsigned long SPARKLE_INTERVAL = 80;
    static constexpr unsigned long CHASE_INTERVAL = 60;
};

#endif // LED_CONTROLLER_H
