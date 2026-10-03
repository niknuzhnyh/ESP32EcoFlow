#pragma once

#include <Arduino.h>

enum LedMode {
    LED_OFF,
    LED_POWER_OK,          // Solid ON: Power is normal
    LED_POWER_LOST_WAIT,   // Slow blink (1s): Power lost, waiting 10m
    LED_POWER_LOST_ALARM,  // Fast strobe (250ms): 10m alarm active
    LED_WIFI_CONNECTING,   // Rapid blink (500ms): Connecting to Wi-Fi
    LED_AP_PORTAL,         // Beacon pulse (1s): SoftAP mode
    LED_SENDING_NOTIF      // Single flash: Request sent
};

class StatusLed {
public:
    static StatusLed& instance();

    void begin(uint8_t pin = 2);
    void setMode(LedMode mode);
    void flashNotification();
    void tick();

private:
    StatusLed() = default;
    void setLed(bool on);

    uint8_t _pin = 2;
    LedMode _mode = LED_OFF;
    uint32_t _lastTick = 0;
    bool _flashActive = false;
    uint32_t _flashUntil = 0;
    uint8_t _step = 0;
};
