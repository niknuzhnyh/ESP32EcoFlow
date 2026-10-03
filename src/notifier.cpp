#include "notifier.h"
#include "config_manager.h"
#include "network_manager.h"
#include "status_led.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

Notifier& Notifier::instance() {
    static Notifier inst;
    return inst;
}

void Notifier::begin() {
    if (_queue == nullptr) {
        _queue = xQueueCreate(10, sizeof(NotificationJob));
        xTaskCreatePinnedToCore(
            workerTask,
            "NotifierWorker",
            8192,
            this,
            1,
            nullptr,
            0 // Core 0
        );
        Serial.println("[Notifier] Worker task initialized on Core 0");
    }
}

void Notifier::workerTask(void* pvParameters) {
    auto* self = static_cast<Notifier*>(pvParameters);
    NotificationJob job;

    while (true) {
        if (xQueueReceive(self->_queue, &job, portMAX_DELAY) == pdTRUE) {
            self->processJob(job);
        }
    }
}

void Notifier::dispatch(PowerEvent event, uint32_t durationSec) {
    if (_queue == nullptr) return;
    NotificationJob job;
    job.event = event;
    job.durationSec = durationSec;
    job.isTest = false;
    job.customMessage = "";
    xQueueSend(_queue, &job, 0);
}

void Notifier::sendTestNotification(bool testSignal, bool testGoogle) {
    if (_queue == nullptr) return;
    NotificationJob job;
    job.event = PWR_EVENT_NONE;
    job.durationSec = 0;
    job.isTest = true;
    job.customMessage = "Тестове сповіщення від ESP32 Power Monitor";
    xQueueSend(_queue, &job, 0);
}

void Notifier::addLog(const String& msg, bool success) {
    portENTER_CRITICAL(&_logMux);
    if (_logs.size() >= MAX_LOGS) {
        _logs.pop_front();
    }
    uint32_t sec = millis() / 1000;
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u", (sec / 3600) % 24, (sec / 60) % 60, sec % 60);

    LogEntry entry;
    entry.timestamp = String(timeStr);
    entry.message = msg;
    entry.success = success;
    _logs.push_back(entry);
    portEXIT_CRITICAL(&_logMux);

    Serial.printf("[Log %s] %s\n", success ? "OK" : "ERR", msg.c_str());
}

std::deque<LogEntry> Notifier::getLogs() {
    portENTER_CRITICAL(&_logMux);
    std::deque<LogEntry> copy = _logs;
    portEXIT_CRITICAL(&_logMux);
    return copy;
}

void Notifier::processJob(const NotificationJob& job) {
    if (WiFi.status() != WL_CONNECTED) {
        addLog("Сповіщення пропущено: Wi-Fi не підключено", false);
        return;
    }

    StatusLed::instance().flashNotification();
    const auto& cfg = ConfigManager::instance().get();

    String eventName = "";
    String statusStr = "";
    String primaryText = "";
    String backupText = "";

    if (job.isTest) {
        eventName = "TEST";
        statusStr = "TEST";
        primaryText = "🔔 [ТЕСТ] Перевірка зв'язку з ESP32 Power Monitor (Основний номер)";
        backupText = "🔔 [ТЕСТ] Перевірка зв'язку з ESP32 Power Monitor (Резервний номер)";
    } else {
        switch (job.event) {
            case PWR_EVENT_LOST_T0:
                eventName = "POWER_LOST_T0";
                statusStr = "OFF";
                primaryText = cfg.msg_t0_primary;
                backupText = cfg.msg_t0_backup;
                break;

            case PWR_EVENT_LOST_T10:
                eventName = "POWER_LOST_T10";
                statusStr = "OFF";
                primaryText = cfg.msg_t10_primary;
                backupText = cfg.msg_t10_backup;
                break;

            case PWR_EVENT_RESTORED:
                eventName = "POWER_RESTORED";
                statusStr = "ON";
                primaryText = cfg.msg_restored_primary;
                backupText = cfg.msg_restored_backup;
                break;

            default:
                return;
        }
    }

    // 1. Google Sheets Webhook
    if (cfg.google_webhook_url.length() > 5) {
        sendGoogleSheets(eventName, statusStr, job.durationSec, primaryText);
    } else {
        Serial.println("[Notifier] Google Sheets URL is empty, skipping");
    }

    // 2. Signal REST API
    if (cfg.signal_url.length() > 5 && cfg.signal_sender.length() > 0) {
        // Send to primary recipient
        if (cfg.primary_phone.length() > 0 && primaryText.length() > 0) {
            std::vector<String> priRecipients = { cfg.primary_phone };
            sendSignal(primaryText, priRecipients);
        }

        // Send to backup recipients
        if (!cfg.backup_phones.empty() && backupText.length() > 0) {
            sendSignal(backupText, cfg.backup_phones);
        }
    } else {
        Serial.println("[Notifier] Signal API URL or sender not configured, skipping");
    }
}

bool Notifier::sendGoogleSheets(const String& eventStr, const String& statusStr, uint32_t durationSec, const String& message) {
    const auto& cfg = ConfigManager::instance().get();
    String url = cfg.google_webhook_url;
    url.trim();
    if (url.length() < 10) return false;

    WiFiClientSecure client;
    client.setInsecure(); // Google uses valid SSL, but avoids CA bundle overhead
    client.setTimeout(12000);

    HTTPClient http;
    // Disable follow-redirects: Google Apps Script executes doPost() and returns 302 with Location header.
    // Following 302 with a POST causes Google's echo server to return HTTP 400 Bad Request.
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.setTimeout(12000);

    if (!http.begin(client, url)) {
        addLog("Помилка з'єднання з Google Sheets URL", false);
        return false;
    }

    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["event"] = eventStr;
    doc["status"] = statusStr;
    doc["duration_sec"] = durationSec;
    doc["message"] = message;
    String devStr = cfg.hostname;
    String ipStr = NetworkManager::instance().getIpAddress();
    if (ipStr.length() > 0 && ipStr != "0.0.0.0") {
        devStr += " (" + ipStr + ")";
    }
    doc["device"] = devStr;
    doc["uptime_sec"] = millis() / 1000;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);
    // 200..302 is success (Google returns 302 after executing doPost)
    bool ok = (httpCode >= 200 && httpCode <= 302);

    if (ok) {
        addLog("Google Sheets: успішно (" + eventStr + ") HTTP " + String(httpCode), true);
    } else {
        String err = http.errorToString(httpCode);
        addLog("Google Sheets помилка HTTP " + String(httpCode) + " (" + err + ")", false);
    }

    http.end();
    return ok;
}

bool Notifier::sendSignal(const String& message, const std::vector<String>& recipients) {
    if (recipients.empty() || message.length() == 0) return true;

    const auto& cfg = ConfigManager::instance().get();
    String base = cfg.signal_url;
    base.trim();
    if (base.length() < 3) return false;

    // Sanitize scheme
    if (!base.startsWith("http://") && !base.startsWith("https://")) {
        base = "http://" + base;
    }
    while (base.endsWith("/")) {
        base.remove(base.length() - 1);
    }
    String endpoint = base + "/v2/send";

    bool isHttps = base.startsWith("https://");
    HTTPClient http;
    http.setTimeout(10000);

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    bool began = false;

    if (isHttps) {
        secureClient.setInsecure();
        secureClient.setTimeout(10000);
        began = http.begin(secureClient, endpoint);
    } else {
        plainClient.setTimeout(10000);
        began = http.begin(plainClient, endpoint);
    }

    if (!began) {
        addLog("Помилка ініціалізації Signal API (" + endpoint + ")", false);
        return false;
    }

    http.addHeader("Content-Type", "application/json");

    JsonDocument doc;
    doc["message"] = message;
    doc["number"] = cfg.signal_sender;
    JsonArray recArr = doc["recipients"].to<JsonArray>();
    for (const auto& r : recipients) {
        recArr.add(r);
    }

    String requestBody;
    serializeJson(doc, requestBody);

    int httpCode = http.POST(requestBody);
    bool ok = (httpCode >= 200 && httpCode < 300);

    String recipientsList = "";
    for (size_t i = 0; i < recipients.size(); i++) {
        if (i > 0) recipientsList += ", ";
        recipientsList += recipients[i];
    }

    if (ok) {
        addLog("Signal -> [" + recipientsList + "]: надіслано (HTTP " + String(httpCode) + ")", true);
    } else {
        String err = http.errorToString(httpCode);
        addLog("Signal помилка HTTP " + String(httpCode) + " (" + err + ") для [" + recipientsList + "] на " + base, false);
    }

    http.end();
    return ok;
}
