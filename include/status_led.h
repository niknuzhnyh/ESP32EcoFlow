#pragma once

#include <Arduino.h>

enum LedMode {
    LED_OFF,
    LED_POWER_OK,          // Solid / soft pulse green
    LED_POWER_LOST_WAIT,   // Pulsing / blinking red (1s)
    LED_POWER_LOST_ALARM,  // Fast blinking red (250ms)
    LED_WIFI_CONNECTING,   // Blinking blue (500ms)
    LED_AP_PORTAL,         // Solid amber/yellow
    LED_SENDING_NOTIF      // Single flash cyan
};

class StatusLed {
public:
    static StatusLed& instance();

    void begin(uint8_t pin = 21);
    void setMode(LedMode mode);
    void flashNotification();
    void tick();

private:
    StatusLed() = default;
    void setColor(uint8_t r, uint8_t g, uint8_t b);

    uint8_t _pin = 21;
    LedMode _mode = LED_OFF;
    uint32_t _lastTick = 0;
    bool _flashActive = false;
    uint32_t _flashUntil = 0;
    uint8_t _step = 0;
};
