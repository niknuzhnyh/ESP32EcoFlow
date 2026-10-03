#include "status_led.h"

StatusLed& StatusLed::instance() {
    static StatusLed inst;
    return inst;
}

void StatusLed::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
    setColor(0, 0, 0);
}

void StatusLed::setColor(uint8_t r, uint8_t g, uint8_t b) {
    neopixelWrite(_pin, r, g, b);
}

void StatusLed::setMode(LedMode mode) {
    _mode = mode;
    _step = 0;
}

void StatusLed::flashNotification() {
    _flashActive = true;
    _flashUntil = millis() + 300;
    setColor(0, 50, 50); // Cyan flash
}

void StatusLed::tick() {
    uint32_t now = millis();

    if (_flashActive) {
        if (now >= _flashUntil) {
            _flashActive = false;
        } else {
            return; // keep cyan
        }
    }

    if (now - _lastTick < 50) return;
    _lastTick = now;
    _step++;

    switch (_mode) {
        case LED_POWER_OK:
            // Steady subtle green (not too bright to preserve power and eyes)
            setColor(0, 30, 0);
            break;

        case LED_POWER_LOST_WAIT:
            // Slow pulse / blink red (1 sec cycle: 500ms ON, 500ms OFF)
            if ((_step % 20) < 10) {
                setColor(50, 0, 0);
            } else {
                setColor(0, 0, 0);
            }
            break;

        case LED_POWER_LOST_ALARM:
            // Fast blink red (250ms cycle: 100ms ON, 150ms OFF)
            if ((_step % 5) < 2) {
                setColor(80, 0, 0);
            } else {
                setColor(0, 0, 0);
            }
            break;

        case LED_WIFI_CONNECTING:
            // Blinking blue (500ms cycle)
            if ((_step % 10) < 5) {
                setColor(0, 0, 40);
            } else {
                setColor(0, 0, 0);
            }
            break;

        case LED_AP_PORTAL:
            // Solid warm amber / orange
            setColor(40, 20, 0);
            break;

        case LED_OFF:
        default:
            setColor(0, 0, 0);
            break;
    }
}
