#include <Arduino.h>
#include "config_manager.h"
#include "status_led.h"
#include "power_monitor.h"
#include "notifier.h"
#include "network_manager.h"
#include "web_server.h"

void setup() {
    Serial.begin(115200);
    delay(1000); // Allow USB CDC to initialize on ESP32-S3

    Serial.println("\n=========================================");
    Serial.println("  ESP32 Power Grid Monitor v1.0 (DevKit V1)  ");
    Serial.println("=========================================");

    // 1. Initialize built-in Status LED (GPIO 2 on ESP32 DevKit V1)
    StatusLed::instance().begin(2);

    // 2. Initialize LittleFS and Configuration
    if (!ConfigManager::instance().begin()) {
        Serial.println("[Main] Failed to initialize ConfigManager!");
    }

    // 3. Initialize Background Notifier Task (FreeRTOS Core 0)
    Notifier::instance().begin();

    // 4. Initialize Hardware Power Monitor (GPIO 4 by default)
    PowerMonitor::instance().begin();

    // Attach Event Callback: Dispatch notifications when power state changes
    PowerMonitor::instance().onEvent([](PowerEvent event, uint32_t outageDurationSec) {
        Notifier::instance().dispatch(event, outageDurationSec);
    });

    // 5. Initialize Wi-Fi / SoftAP Manager & mDNS
    NetworkManager::instance().begin();

    // 6. Initialize Web Server, Captive Portal, and OTA
    WebServerManager::instance().begin();

    Serial.println("[Main] System initialized and running.");
}

void loop() {
    // Non-blocking state updates
    StatusLed::instance().tick();
    PowerMonitor::instance().tick();
    NetworkManager::instance().tick();
    WebServerManager::instance().tick();
}
