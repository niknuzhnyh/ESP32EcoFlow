#pragma once

#include <Arduino.h>
#include <functional>

enum PowerState {
    PWR_STATE_INIT,
    PWR_STATE_OK,            // 5V present
    PWR_STATE_LOST_WAITING,  // 5V lost, T=0 sent, counting down to 10 min
    PWR_STATE_LOST_ALARM     // 5V lost >= 10 min, T=10m sent
};

enum PowerEvent {
    PWR_EVENT_NONE,
    PWR_EVENT_LOST_T0,       // Initial power drop
    PWR_EVENT_LOST_T10,      // 10 minutes elapsed with no power
    PWR_EVENT_RESTORED       // Power restored
};

typedef std::function<void(PowerEvent event, uint32_t outageDurationSec)> PowerEventCallback;

class PowerMonitor {
public:
    static PowerMonitor& instance();

    void begin();
    void tick();

    bool isPowerPresent() const;
    PowerState getState() const;
    const char* getStateStr() const;
    uint32_t getStateDurationSec() const;
    uint32_t getCountdownRemainingSec() const;
    int getRawPinValue() const;

    void onEvent(PowerEventCallback cb);

private:
    PowerMonitor() = default;

    uint8_t _pin = 4;
    bool _inverted = false;
    uint32_t _debounceMs = 2000;
    uint32_t _alarmTimeoutSec = 600;

    bool _rawState = false;
    bool _debouncedState = false;
    uint32_t _lastRawChangeMs = 0;

    bool _isPendingChange = false;
    bool _pendingState = false;
    uint32_t _pendingChangeMs = 0;

    PowerState _state = PWR_STATE_INIT;
    uint32_t _stateChangeMs = 0;
    uint32_t _powerLostMs = 0;
    bool _alarmTriggered = false;

    PowerEventCallback _eventCallback = nullptr;

    bool readPinState() const;
};
