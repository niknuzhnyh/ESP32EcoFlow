#include "serial_cli.h"
#include "config_manager.h"
#include "network_manager.h"
#include "power_monitor.h"
#include "notifier.h"
#include <WiFi.h>

SerialCli& SerialCli::instance() {
    static SerialCli inst;
    return inst;
}

void SerialCli::begin() {
    _inputBuffer.reserve(128);
    printBanner();
}

void SerialCli::printBanner() {
    const auto& cfg = ConfigManager::instance().get();
    Serial.println("\n=============================================================");
    Serial.println("  [*] ESP32 Power Grid Monitor v1.0 (DevKit V1)");
    Serial.println("=============================================================");
    if (NetworkManager::instance().isApMode()) {
        Serial.printf("  [-] Wi-Fi Mode:    SoftAP (Access Point)\n");
        Serial.printf("  [-] AP SSID:       %s (No Password)\n", cfg.ap_ssid.c_str());
        Serial.printf("  [-] Web UI URL:    http://%s/\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("  [-] DNS Server:    %s:53 (Captive Portal)\n", WiFi.softAPIP().toString().c_str());
    } else {
        Serial.printf("  [-] Wi-Fi Mode:    Station (Connected)\n");
        Serial.printf("  [-] SSID:          %s\n", cfg.wifi_ssid.c_str());
        Serial.printf("  [-] Web UI URL:    http://%s/ (or http://%s.local)\n",
                      WiFi.localIP().toString().c_str(), cfg.hostname.c_str());
    }
    Serial.printf("  [-] Sense Pin:     GPIO %d\n", cfg.sense_gpio);
    Serial.printf("  [-] Status LED:    GPIO 2\n");
    Serial.println("=============================================================");
    Serial.println("Type 'help' in this terminal for available commands.\n");
    Serial.print("ESP32> ");
}

void SerialCli::printHelp() {
    Serial.println("\n--- [ Available CLI Commands ] ---");
    Serial.println("  status             - Show current system, power, Wi-Fi & IP info");
    Serial.println("  ip                 - Show current IP address and Web UI URL");
    Serial.println("  scan               - Scan for available 2.4GHz Wi-Fi networks");
    Serial.println("  wifi <ssid> <pass> - Connect directly to your home Wi-Fi");
    Serial.println("  test               - Trigger test notification (Signal / Sheets)");
    Serial.println("  reboot             - Reboot ESP32");
    Serial.println("  help               - Show this help menu");
    Serial.println("----------------------------------");
}

void SerialCli::printStatus() {
    const auto& nm = NetworkManager::instance();
    const auto& pm = PowerMonitor::instance();

    Serial.println("\n--- [ System Status ] ---");
    Serial.printf("  Uptime:       %lu sec\n", millis() / 1000);
    Serial.printf("  Free Heap:    %u bytes\n", ESP.getFreeHeap());
    Serial.printf("  Power State:  %s (Raw GPIO 4: %d, Outage: %lu s)\n",
                  pm.getStateStr(), pm.getRawPinValue(), pm.getStateDurationSec());

    if (nm.isApMode()) {
        Serial.printf("  Wi-Fi Mode:   SoftAP (SSID: '%s', IP: %s)\n",
                      nm.getSsid().c_str(), nm.getIpAddress().c_str());
        Serial.printf("  AP Clients:   %d station(s) connected\n", WiFi.softAPgetStationNum());
    } else if (nm.isConnected()) {
        Serial.printf("  Wi-Fi Mode:   Station (SSID: '%s', IP: %s, RSSI: %d dBm)\n",
                      nm.getSsid().c_str(), nm.getIpAddress().c_str(), nm.getRssi());
    } else {
        Serial.println("  Wi-Fi Mode:   Connecting...");
    }
    Serial.println("-------------------------");
}

void SerialCli::scanWifi() {
    Serial.println("\n[Scan] Scanning for Wi-Fi networks...");
    int n = WiFi.scanNetworks(false, true); // active scan
    if (n <= 0) {
        Serial.println("[Scan] No networks found or scan failed.");
    } else {
        Serial.printf("[Scan] Found %d networks:\n", n);
        for (int i = 0; i < n; i++) {
            const char* enc = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted";
            Serial.printf("  %2d: %-25s (Ch: %2d, RSSI: %3d dBm, %s)\n",
                          i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), enc);
        }
    }
    WiFi.scanDelete();
}

void SerialCli::connectWifi(const String& args) {
    int spaceIdx = args.indexOf(' ');
    String ssid, pass;
    if (spaceIdx == -1) {
        ssid = args;
        pass = "";
    } else {
        ssid = args.substring(0, spaceIdx);
        pass = args.substring(spaceIdx + 1);
    }
    ssid.trim();
    pass.trim();

    if (ssid.length() == 0) {
        Serial.println("[CLI] Error: Usage: wifi <ssid> <password>");
        return;
    }

    Serial.printf("[CLI] Saving Wi-Fi: SSID '%s', Password '%s'...\n",
                  ssid.c_str(), pass.length() > 0 ? "********" : "(none)");

    auto& cfg = ConfigManager::instance().get();
    cfg.wifi_ssid = ssid;
    cfg.wifi_password = pass;
    ConfigManager::instance().save();

    Serial.println("[CLI] Credentials saved to LittleFS. Connecting now...");
    NetworkManager::instance().begin();
}

void SerialCli::processCommand(const String& cmdLine) {
    String line = cmdLine;
    line.trim();
    if (line.length() == 0) {
        Serial.print("ESP32> ");
        return;
    }

    int spaceIdx = line.indexOf(' ');
    String cmd = (spaceIdx == -1) ? line : line.substring(0, spaceIdx);
    String args = (spaceIdx == -1) ? "" : line.substring(spaceIdx + 1);
    cmd.toLowerCase();
    args.trim();

    if (cmd == "help" || cmd == "?") {
        printHelp();
    } else if (cmd == "status") {
        printStatus();
    } else if (cmd == "ip") {
        Serial.printf("\n[IP] Current IP: %s\n", NetworkManager::instance().getIpAddress().c_str());
        Serial.printf("[IP] Web UI: http://%s/\n\n", NetworkManager::instance().getIpAddress().c_str());
    } else if (cmd == "scan") {
        scanWifi();
    } else if (cmd == "wifi") {
        connectWifi(args);
    } else if (cmd == "test") {
        Serial.println("[CLI] Triggering test notification...");
        Notifier::instance().sendTestNotification(true, true);
    } else if (cmd == "reboot" || cmd == "restart") {
        Serial.println("[CLI] Rebooting ESP32...");
        delay(300);
        ESP.restart();
    } else {
        Serial.printf("[CLI] Unknown command: '%s'. Type 'help' for commands.\n", cmd.c_str());
    }

    Serial.print("ESP32> ");
}

void SerialCli::tick() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n') {
            if (_inputBuffer.length() > 0) {
                Serial.println();
                processCommand(_inputBuffer);
                _inputBuffer = "";
            } else {
                Serial.println();
                Serial.print("ESP32> ");
            }
        } else if (c == '\b' || c == 127) { // Backspace
            if (_inputBuffer.length() > 0) {
                _inputBuffer.remove(_inputBuffer.length() - 1);
                Serial.print("\b \b");
            }
        } else {
            _inputBuffer += c;
            Serial.print(c); // Echo typed char
        }
    }
}
