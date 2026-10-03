#include "web_server.h"
#include "web_assets.h"
#include "config_manager.h"
#include "power_monitor.h"
#include "network_manager.h"
#include "notifier.h"
#include <Update.h>
#include <ArduinoJson.h>

WebServerManager& WebServerManager::instance() {
    static WebServerManager inst;
    return inst;
}

void WebServerManager::begin() {
    setupRoutes();
    _server.begin();
    Serial.println("[WebServer] HTTP server started on port 80");
}

void WebServerManager::setupRoutes() {
    // Web UI
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });

    // REST API
    _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    _server.on("/api/config", HTTP_POST, [this]() { handleSaveConfig(); });
    _server.on("/api/test", HTTP_POST, [this]() { handleTestAlert(); });
    _server.on("/api/reboot", HTTP_POST, [this]() { handleReboot(); });
    _server.on("/api/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });

    // OTA firmware update endpoints
    _server.on("/update", HTTP_GET, [this]() {
        _server.sendHeader("Connection", "close");
        _server.send(200, "text/html", INDEX_HTML);
    });

    _server.on("/update", HTTP_POST, [this]() {
        _server.sendHeader("Connection", "close");
        if (Update.hasError()) {
            _server.send(500, "text/plain", "OTA Failed: " + String(Update.errorString()));
        } else {
            _server.send(200, "text/plain", "OK");
            _rebootScheduled = true;
            _rebootAtMs = millis() + 1500;
        }
    }, [this]() {
        // Upload handler callback
        HTTPUpload& upload = _server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("[OTA] Update Start: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                Serial.printf("[OTA] Update Success: %u bytes\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    // Captive Portal probes
    _server.on("/generate_204", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/gen_204", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/ncsi.txt", HTTP_GET, [this]() { _server.send(200, "text/plain", "Microsoft NCSI"); });
    _server.on("/connecttest.txt", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/hotspot-detect.html", HTTP_GET, [this]() { handleRoot(); });

    // Catch-all
    _server.onNotFound([this]() { handleNotFound(); });
}

void WebServerManager::handleRoot() {
    _server.sendHeader("Content-Type", "text/html; charset=utf-8");
    _server.send(200, "text/html", INDEX_HTML);
}

void WebServerManager::handleStatus() {
    const auto& pm = PowerMonitor::instance();
    const auto& nm = NetworkManager::instance();
    const auto& logs = Notifier::instance().getLogs();

    JsonDocument doc;
    doc["power_present"] = pm.isPowerPresent();
    doc["raw_gpio"] = pm.getRawPinValue();
    doc["state"] = pm.getStateStr();
    doc["duration_sec"] = pm.getStateDurationSec();
    doc["countdown_sec"] = pm.getCountdownRemainingSec();

    doc["ssid"] = nm.getSsid();
    doc["ip"] = nm.getIpAddress();
    doc["rssi"] = nm.getRssi();
    doc["uptime_sec"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();

    JsonArray logArr = doc["logs"].to<JsonArray>();
    for (const auto& l : logs) {
        JsonObject obj = logArr.add<JsonObject>();
        obj["time"] = l.timestamp;
        obj["msg"] = l.message;
        obj["success"] = l.success;
    }

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
}

void WebServerManager::handleGetConfig() {
    String jsonStr = ConfigManager::instance().serializeJson(true);
    _server.send(200, "application/json", jsonStr);
}

void WebServerManager::handleSaveConfig() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"Missing body\"}");
        return;
    }

    String body = _server.arg("plain");
    bool ok = ConfigManager::instance().deserializeJson(body);

    if (ok) {
        // Re-init power monitor with new pin / debounce / timings if changed
        PowerMonitor::instance().begin();
        _server.send(200, "application/json", "{\"status\":\"saved\"}");
    } else {
        _server.send(400, "application/json", "{\"error\":\"Failed to parse JSON\"}");
    }
}

void WebServerManager::handleTestAlert() {
    Notifier::instance().sendTestNotification(true, true);
    _server.send(200, "application/json", "{\"status\":\"queued\"}");
}

void WebServerManager::handleReboot() {
    _server.send(200, "application/json", "{\"status\":\"rebooting\"}");
    _rebootScheduled = true;
    _rebootAtMs = millis() + 1000;
}

void WebServerManager::handleFactoryReset() {
    ConfigManager::instance().resetToDefaults();
    _server.send(200, "application/json", "{\"status\":\"resetting\"}");
    _rebootScheduled = true;
    _rebootAtMs = millis() + 1000;
}

void WebServerManager::handleNotFound() {
    if (NetworkManager::instance().isApMode()) {
        String url = "http://";
        url += NetworkManager::instance().getIpAddress();
        url += "/";
        _server.sendHeader("Location", url, true);
        _server.send(302, "text/plain", "");
        return;
    }
    _server.send(404, "text/plain", "404 Not Found");
}

void WebServerManager::tick() {
    _server.handleClient();

    if (_rebootScheduled && millis() >= _rebootAtMs) {
        Serial.println("[WebServer] Performing scheduled restart...");
        ESP.restart();
    }
}
