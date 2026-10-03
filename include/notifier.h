#pragma once

#include <Arduino.h>
#include <vector>
#include <deque>
#include "power_monitor.h"

struct NotificationJob {
    PowerEvent event;
    uint32_t durationSec;
    bool isTest;
    String customMessage;
};

struct LogEntry {
    String timestamp;
    String message;
    bool success;
};

class Notifier {
public:
    static Notifier& instance();

    void begin();
    void dispatch(PowerEvent event, uint32_t durationSec);
    void sendTestNotification(bool testSignal = true, bool testGoogle = true);

    std::deque<LogEntry> getLogs();
    void addLog(const String& msg, bool success);

private:
    Notifier() = default;

    static void workerTask(void* pvParameters);
    void processJob(const NotificationJob& job);

    bool sendGoogleSheets(const String& eventStr, const String& statusStr, uint32_t durationSec, const String& message);
    bool sendSignal(const String& message, const std::vector<String>& recipients);

    QueueHandle_t _queue = nullptr;
    portMUX_TYPE _logMux = portMUX_INITIALIZER_UNLOCKED;
    std::deque<LogEntry> _logs;
    const size_t MAX_LOGS = 25;
};
