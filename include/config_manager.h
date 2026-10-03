#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

struct AppConfig {
    // Wi-Fi Settings
    String wifi_ssid = "";
    String wifi_password = "";
    String ap_ssid = "PowerMonitor-Setup";
    String ap_password = ""; // Empty = open network
    String hostname = "power-monitor";

    // Signal REST API Settings
    String signal_url = "http://192.168.1.100:8080";
    String signal_sender = ""; // e.g. "+380123456789"

    // Google Sheets Webhook URL
    String google_webhook_url = ""; // e.g. "https://script.google.com/macros/s/XXXX/exec"

    // Phone numbers
    String primary_phone = ""; // e.g. "+380671112233"
    std::vector<String> backup_phones; // e.g. ["+380501112233", "+380931112233"]

    // Message Templates
    // T = 0 (Power loss)
    String msg_t0_primary = "⚠️ [СВІТЛО ЗНИКЛО] Зафіксовано відключення напруги 5V USB. Таймер очікування 10 хв запущено.";
    String msg_t0_backup = "⚠️ [СВІТЛО ЗНИКЛО] В мережі зникло живлення 220V/5V. Очікуємо відновлення.";

    // T = 10 min (Still no power after 10 min)
    String msg_t10_primary = "🚨 [ТРИВОГА 10 ХВ] Світла немає вже 10 хвилин! Можливо тривале знеструмлення. Перевірте стан систем.";
    String msg_t10_backup = "🚨 [УВАГА] Світло не відновилося протягом 10 хвилин. Спрацював тривожний таймер.";

    // Power Restored
    String msg_restored_primary = "✅ [СВІТЛО З'ЯВИЛОСЯ] Електропостачання відновлено! Живлення 5V USB стабільне.";
    String msg_restored_backup = "✅ [СВІТЛО З'ЯВИЛОСЯ] Напругу в електромережі відновлено.";

    // Hardware & Timing
    uint8_t sense_gpio = 4;        // Default GPIO4 for 5V sense
    bool sense_inverted = false;   // false = HIGH means 5V present, true = LOW means 5V present
    uint32_t debounce_ms = 2000;   // 2 seconds debounce filter
    uint32_t alarm_timeout_sec = 600; // 10 minutes = 600s
};

class ConfigManager {
public:
    static ConfigManager& instance();

    bool begin();
    bool load();
    bool save();
    void resetToDefaults();

    AppConfig& get();
    String serializeJson(bool maskSecrets = false);
    bool deserializeJson(const String& jsonStr);

    String getBackupPhonesCsv() const;
    void setBackupPhonesFromCsv(const String& csv);

private:
    ConfigManager() = default;
    AppConfig _config;
    const char* _configFilePath = "/config.json";
};
