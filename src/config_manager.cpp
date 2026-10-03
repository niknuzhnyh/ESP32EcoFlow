#include "config_manager.h"
#include <LittleFS.h>

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Config] LittleFS mount failed, formatting...");
        return false;
    }
    Serial.println("[Config] LittleFS mounted successfully");
    return load();
}

AppConfig& ConfigManager::get() {
    return _config;
}

String ConfigManager::getBackupPhonesCsv() const {
    String csv = "";
    for (size_t i = 0; i < _config.backup_phones.size(); i++) {
        if (i > 0) csv += ", ";
        csv += _config.backup_phones[i];
    }
    return csv;
}

void ConfigManager::setBackupPhonesFromCsv(const String& csv) {
    _config.backup_phones.clear();
    int start = 0;
    while (start < csv.length()) {
        int comma = csv.indexOf(',', start);
        String phone;
        if (comma == -1) {
            phone = csv.substring(start);
            start = csv.length();
        } else {
            phone = csv.substring(start, comma);
            start = comma + 1;
        }
        phone.trim();
        if (phone.length() > 0) {
            _config.backup_phones.push_back(phone);
        }
    }
}

void ConfigManager::resetToDefaults() {
    _config = AppConfig();
    save();
}

bool ConfigManager::load() {
    if (!LittleFS.exists(_configFilePath)) {
        Serial.println("[Config] Config file not found, creating default");
        return save();
    }

    File file = LittleFS.open(_configFilePath, "r");
    if (!file) {
        Serial.println("[Config] Failed to open config file for reading");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = ::deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[Config] JSON deserialization failed: %s\n", error.c_str());
        return false;
    }

    _config.wifi_ssid = doc["wifi_ssid"] | "";
    _config.wifi_password = doc["wifi_password"] | "";
    _config.ap_ssid = doc["ap_ssid"] | "PowerMonitor-Setup";
    _config.ap_password = doc["ap_password"] | "";
    _config.hostname = doc["hostname"] | "power-monitor";

    _config.signal_url = doc["signal_url"] | "http://192.168.1.100:8080";
    _config.signal_sender = doc["signal_sender"] | "";

    _config.google_webhook_url = doc["google_webhook_url"] | "";

    _config.primary_phone = doc["primary_phone"] | "";
    _config.backup_phones.clear();
    JsonArray backups = doc["backup_phones"].as<JsonArray>();
    for (JsonVariant v : backups) {
        String phone = v.as<String>();
        phone.trim();
        if (phone.length() > 0) {
            _config.backup_phones.push_back(phone);
        }
    }

    _config.msg_t0_primary = doc["msg_t0_primary"] | _config.msg_t0_primary;
    _config.msg_t0_backup = doc["msg_t0_backup"] | _config.msg_t0_backup;
    _config.msg_t10_primary = doc["msg_t10_primary"] | _config.msg_t10_primary;
    _config.msg_t10_backup = doc["msg_t10_backup"] | _config.msg_t10_backup;
    _config.msg_restored_primary = doc["msg_restored_primary"] | _config.msg_restored_primary;
    _config.msg_restored_backup = doc["msg_restored_backup"] | _config.msg_restored_backup;

    _config.sense_gpio = doc["sense_gpio"] | 4;
    _config.sense_inverted = doc["sense_inverted"] | false;
    _config.debounce_ms = doc["debounce_ms"] | 2000;
    _config.alarm_timeout_sec = doc["alarm_timeout_sec"] | 600;

    Serial.println("[Config] Configuration loaded successfully");
    return true;
}

bool ConfigManager::save() {
    File file = LittleFS.open(_configFilePath, "w");
    if (!file) {
        Serial.println("[Config] Failed to open config file for writing");
        return false;
    }

    JsonDocument doc;
    doc["wifi_ssid"] = _config.wifi_ssid;
    doc["wifi_password"] = _config.wifi_password;
    doc["ap_ssid"] = _config.ap_ssid;
    doc["ap_password"] = _config.ap_password;
    doc["hostname"] = _config.hostname;

    doc["signal_url"] = _config.signal_url;
    doc["signal_sender"] = _config.signal_sender;

    doc["google_webhook_url"] = _config.google_webhook_url;

    doc["primary_phone"] = _config.primary_phone;
    JsonArray backups = doc["backup_phones"].to<JsonArray>();
    for (const auto& phone : _config.backup_phones) {
        backups.add(phone);
    }

    doc["msg_t0_primary"] = _config.msg_t0_primary;
    doc["msg_t0_backup"] = _config.msg_t0_backup;
    doc["msg_t10_primary"] = _config.msg_t10_primary;
    doc["msg_t10_backup"] = _config.msg_t10_backup;
    doc["msg_restored_primary"] = _config.msg_restored_primary;
    doc["msg_restored_backup"] = _config.msg_restored_backup;

    doc["sense_gpio"] = _config.sense_gpio;
    doc["sense_inverted"] = _config.sense_inverted;
    doc["debounce_ms"] = _config.debounce_ms;
    doc["alarm_timeout_sec"] = _config.alarm_timeout_sec;

    if (serializeJsonPretty(doc, file) == 0) {
        Serial.println("[Config] Failed to write JSON to file");
        file.close();
        return false;
    }

    file.close();
    Serial.println("[Config] Configuration saved to LittleFS");
    return true;
}

String ConfigManager::serializeJson(bool maskSecrets) {
    JsonDocument doc;
    doc["wifi_ssid"] = _config.wifi_ssid;
    doc["wifi_password"] = maskSecrets && (_config.wifi_password.length() > 0) ? "********" : _config.wifi_password;
    doc["ap_ssid"] = _config.ap_ssid;
    doc["ap_password"] = maskSecrets && (_config.ap_password.length() > 0) ? "********" : _config.ap_password;
    doc["hostname"] = _config.hostname;

    doc["signal_url"] = _config.signal_url;
    doc["signal_sender"] = _config.signal_sender;

    doc["google_webhook_url"] = _config.google_webhook_url;

    doc["primary_phone"] = _config.primary_phone;
    JsonArray backups = doc["backup_phones"].to<JsonArray>();
    for (const auto& phone : _config.backup_phones) {
        backups.add(phone);
    }
    doc["backup_phones_csv"] = getBackupPhonesCsv();

    doc["msg_t0_primary"] = _config.msg_t0_primary;
    doc["msg_t0_backup"] = _config.msg_t0_backup;
    doc["msg_t10_primary"] = _config.msg_t10_primary;
    doc["msg_t10_backup"] = _config.msg_t10_backup;
    doc["msg_restored_primary"] = _config.msg_restored_primary;
    doc["msg_restored_backup"] = _config.msg_restored_backup;

    doc["sense_gpio"] = _config.sense_gpio;
    doc["sense_inverted"] = _config.sense_inverted;
    doc["debounce_ms"] = _config.debounce_ms;
    doc["alarm_timeout_sec"] = _config.alarm_timeout_sec;

    String output;
    ::serializeJson(doc, output);
    return output;
}

bool ConfigManager::deserializeJson(const String& jsonStr) {
    JsonDocument doc;
    DeserializationError error = ::deserializeJson(doc, jsonStr);
    if (error) {
        Serial.printf("[Config] Deserialization error: %s\n", error.c_str());
        return false;
    }

    if (doc["wifi_ssid"].is<const char*>()) _config.wifi_ssid = doc["wifi_ssid"].as<String>();
    if (doc["wifi_password"].is<const char*>() && doc["wifi_password"].as<String>() != "********") {
        _config.wifi_password = doc["wifi_password"].as<String>();
    }
    if (doc["ap_ssid"].is<const char*>()) _config.ap_ssid = doc["ap_ssid"].as<String>();
    if (doc["ap_password"].is<const char*>() && doc["ap_password"].as<String>() != "********") {
        _config.ap_password = doc["ap_password"].as<String>();
    }
    if (doc["hostname"].is<const char*>()) _config.hostname = doc["hostname"].as<String>();

    if (doc["signal_url"].is<const char*>()) _config.signal_url = doc["signal_url"].as<String>();
    if (doc["signal_sender"].is<const char*>()) _config.signal_sender = doc["signal_sender"].as<String>();

    if (doc["google_webhook_url"].is<const char*>()) _config.google_webhook_url = doc["google_webhook_url"].as<String>();

    if (doc["primary_phone"].is<const char*>()) _config.primary_phone = doc["primary_phone"].as<String>();

    if (doc["backup_phones_csv"].is<const char*>()) {
        setBackupPhonesFromCsv(doc["backup_phones_csv"].as<String>());
    } else if (doc["backup_phones"].is<JsonArray>()) {
        _config.backup_phones.clear();
        for (JsonVariant v : doc["backup_phones"].as<JsonArray>()) {
            String p = v.as<String>();
            p.trim();
            if (p.length() > 0) _config.backup_phones.push_back(p);
        }
    }

    if (doc["msg_t0_primary"].is<const char*>()) _config.msg_t0_primary = doc["msg_t0_primary"].as<String>();
    if (doc["msg_t0_backup"].is<const char*>()) _config.msg_t0_backup = doc["msg_t0_backup"].as<String>();
    if (doc["msg_t10_primary"].is<const char*>()) _config.msg_t10_primary = doc["msg_t10_primary"].as<String>();
    if (doc["msg_t10_backup"].is<const char*>()) _config.msg_t10_backup = doc["msg_t10_backup"].as<String>();
    if (doc["msg_restored_primary"].is<const char*>()) _config.msg_restored_primary = doc["msg_restored_primary"].as<String>();
    if (doc["msg_restored_backup"].is<const char*>()) _config.msg_restored_backup = doc["msg_restored_backup"].as<String>();

    if (doc["sense_gpio"].is<uint8_t>()) _config.sense_gpio = doc["sense_gpio"].as<uint8_t>();
    if (doc["sense_inverted"].is<bool>()) _config.sense_inverted = doc["sense_inverted"].as<bool>();
    if (doc["debounce_ms"].is<uint32_t>()) _config.debounce_ms = doc["debounce_ms"].as<uint32_t>();
    if (doc["alarm_timeout_sec"].is<uint32_t>()) _config.alarm_timeout_sec = doc["alarm_timeout_sec"].as<uint32_t>();

    return save();
}
