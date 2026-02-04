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
 * @brief Predefined status colors for visual feedback
 */
namespace StatusColor
{
    constexpr uint8_t CONNECTING_R = 122;
    constexpr uint8_t CONNECTING_G = 122;
    constexpr uint8_t CONNECTING_B = 0;

    constexpr uint8_t SUCCESS_R = 0;
    constexpr uint8_t SUCCESS_G = 255;
    constexpr uint8_t SUCCESS_B = 0;

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

    static constexpr uint8_t ANIMATION_DELAY = 50;
};

#endif // LED_CONTROLLER_H
