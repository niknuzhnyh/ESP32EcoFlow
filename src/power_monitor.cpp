#include "power_monitor.h"
#include "config_manager.h"
#include "status_led.h"

PowerMonitor& PowerMonitor::instance() {
    static PowerMonitor inst;
    return inst;
}

void PowerMonitor::begin() {
    const auto& cfg = ConfigManager::instance().get();
    _pin = cfg.sense_gpio;
    _inverted = cfg.sense_inverted;
    _debounceMs = cfg.debounce_ms;
    _alarmTimeoutSec = cfg.alarm_timeout_sec;

    if (_inverted) {
        pinMode(_pin, INPUT_PULLUP);
    } else {
        pinMode(_pin, INPUT_PULLDOWN);
    }

    _rawState = readPinState();
    _debouncedState = _rawState;
    _lastRawChangeMs = millis();
    _isPendingChange = false;
    _pendingChangeMs = 0;
    _stateChangeMs = millis();
    _alarmTriggered = false;

    if (_debouncedState) {
        _state = PWR_STATE_OK;
        StatusLed::instance().setMode(LED_POWER_OK);
        Serial.printf("[PowerMonitor] Initial state: POWER PRESENT (GPIO %d = %d)\n", _pin, getRawPinValue());
    } else {
        // If booted without power:
        _state = PWR_STATE_LOST_WAITING;
        _powerLostMs = millis();
        StatusLed::instance().setMode(LED_POWER_LOST_WAIT);
        Serial.printf("[PowerMonitor] Initial state: NO POWER (GPIO %d = %d)\n", _pin, getRawPinValue());
    }
}

bool PowerMonitor::readPinState() const {
    // Multi-sample filtering: 7 samples spaced by 100 microseconds to suppress noise spikes
    int highCount = 0;
    for (int i = 0; i < 7; i++) {
        if (digitalRead(_pin) == HIGH) highCount++;
        delayMicroseconds(100);
    }
    bool isHigh = (highCount >= 4);
    // If inverted: LOW means power is ON. If not inverted: HIGH means power is ON.
    return _inverted ? !isHigh : isHigh;
}

int PowerMonitor::getRawPinValue() const {
    return digitalRead(_pin);
}

bool PowerMonitor::isPowerPresent() const {
    return _debouncedState;
}

PowerState PowerMonitor::getState() const {
    return _state;
}

const char* PowerMonitor::getStateStr() const {
    switch (_state) {
        case PWR_STATE_OK: return "OK";
        case PWR_STATE_LOST_WAITING: return "LOST_WAITING";
        case PWR_STATE_LOST_ALARM: return "LOST_ALARM";
        default: return "INIT";
    }
}

uint32_t PowerMonitor::getStateDurationSec() const {
    return (millis() - _stateChangeMs) / 1000;
}

uint32_t PowerMonitor::getCountdownRemainingSec() const {
    if (_state != PWR_STATE_LOST_WAITING) return 0;
    uint32_t elapsed = (millis() - _powerLostMs) / 1000;
    if (elapsed >= _alarmTimeoutSec) return 0;
    return _alarmTimeoutSec - elapsed;
}

void PowerMonitor::onEvent(PowerEventCallback cb) {
    _eventCallback = cb;
}

void PowerMonitor::tick() {
    uint32_t now = millis();
    bool currentRaw = readPinState();
    _rawState = currentRaw;

    // Check if the raw pin differs from the currently confirmed debounced state
    if (currentRaw != _debouncedState) {
        if (!_isPendingChange || currentRaw != _pendingState) {
            // New transition candidate detected
            _isPendingChange = true;
            _pendingState = currentRaw;
            _pendingChangeMs = now;
        } else if ((now - _pendingChangeMs) >= _debounceMs) {
            // State has remained stable for the full debounce duration!
            _debouncedState = _pendingState;
            _isPendingChange = false;

            Serial.printf("[PowerMonitor] Confirmed state change! Power present: %s (Raw GPIO: %d)\n",
                          _debouncedState ? "YES" : "NO", digitalRead(_pin));

            if (!_debouncedState) {
                // Power lost!
                _state = PWR_STATE_LOST_WAITING;
                _stateChangeMs = now;
                _powerLostMs = now;
                _alarmTriggered = false;
                StatusLed::instance().setMode(LED_POWER_LOST_WAIT);

                if (_eventCallback) {
                    _eventCallback(PWR_EVENT_LOST_T0, 0);
                }
            } else {
                // Power restored!
                uint32_t outageDuration = (_powerLostMs > 0) ? (now - _powerLostMs) / 1000 : 0;
                _state = PWR_STATE_OK;
                _stateChangeMs = now;
                _powerLostMs = 0;
                _alarmTriggered = false;
                StatusLed::instance().setMode(LED_POWER_OK);

                if (_eventCallback) {
                    _eventCallback(PWR_EVENT_RESTORED, outageDuration);
                }
            }
        }
    } else {
        // Raw state matches confirmed state: cancel transient fluctuations
        _isPendingChange = false;
    }

    // Check 10-minute alarm timer if power is lost
    if (_state == PWR_STATE_LOST_WAITING && !_alarmTriggered) {
        uint32_t elapsed = (now - _powerLostMs) / 1000;
        if (elapsed >= _alarmTimeoutSec) {
            _alarmTriggered = true;
            _state = PWR_STATE_LOST_ALARM;
            StatusLed::instance().setMode(LED_POWER_LOST_ALARM);
            Serial.println("[PowerMonitor] 10-minute alarm triggered: Power still not restored!");

            if (_eventCallback) {
                _eventCallback(PWR_EVENT_LOST_T10, elapsed);
            }
        }
    }
}
