#include "status_led.h"

StatusLed& StatusLed::instance() {
    static StatusLed inst;
    return inst;
}

void StatusLed::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
    setLed(false);
}

void StatusLed::setLed(bool on) {
    digitalWrite(_pin, on ? HIGH : LOW);
}

void StatusLed::setMode(LedMode mode) {
    _mode = mode;
    _step = 0;
}

void StatusLed::flashNotification() {
    _flashActive = true;
    _flashUntil = millis() + 250;
    setLed(true);
}

void StatusLed::tick() {
    uint32_t now = millis();

    if (_flashActive) {
        if (now >= _flashUntil) {
            _flashActive = false;
        } else {
            setLed(true);
            return;
        }
    }

    if (now - _lastTick < 50) return;
    _lastTick = now;
    _step++;

    switch (_mode) {
        case LED_POWER_OK:
            // Solid ON: 5V is present, everything is operating normally
            setLed(true);
            break;

        case LED_POWER_LOST_WAIT:
            // Slow pulse / blink (1 sec cycle: 500ms ON, 500ms OFF)
            setLed((_step % 20) < 10);
            break;

        case LED_POWER_LOST_ALARM:
            // Fast strobe (250ms cycle: 100ms ON, 150ms OFF)
            setLed((_step % 5) < 2);
            break;

        case LED_WIFI_CONNECTING:
            // Rapid blink (500ms cycle: 250ms ON, 250ms OFF)
            setLed((_step % 10) < 5);
            break;

        case LED_AP_PORTAL:
            // Beacon pulse (1 sec cycle: 100ms ON, 900ms OFF)
            setLed((_step % 20) < 2);
            break;

        case LED_OFF:
        default:
            setLed(false);
            break;
    }
}
