#include "network_manager.h"
#include "config_manager.h"
#include "status_led.h"

NetworkManager& NetworkManager::instance() {
    static NetworkManager inst;
    return inst;
}

void NetworkManager::begin() {
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
            Serial.printf("[WiFi-AP] Client associated! MAC: %02X:%02X:%02X:%02X:%02X:%02X, AID: %d\n",
                          info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                          info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                          info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5],
                          info.wifi_ap_staconnected.aid);
        } else if (event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED) {
            Serial.printf("[WiFi-AP] DHCP assigned IP: %s\n",
                          IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
        } else if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
            Serial.printf("[WiFi-AP] Client disconnected! MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                          info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
                          info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
                          info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]);
        }
    });

    const auto& cfg = ConfigManager::instance().get();

    if (cfg.wifi_ssid.length() == 0) {
        Serial.println("[Network] No Wi-Fi SSID configured, launching AP mode");
        startApMode();
        return;
    }

    // Cleanly stop any existing AP and DNS services
    _dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(false);
    delay(100);

    Serial.printf("[Network] Connecting to Wi-Fi SSID: %s\n", cfg.wifi_ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // Disable modem sleep to prevent 4-way handshake timeouts
    WiFi.setHostname(cfg.hostname.c_str());
    WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_password.c_str());

    _state = NET_CONNECTING;
    _connectStartMs = millis();
    StatusLed::instance().setMode(LED_WIFI_CONNECTING);
}

void NetworkManager::startApMode() {
    const auto& cfg = ConfigManager::instance().get();
    _dnsServer.stop();
    WiFi.disconnect(false);
    delay(100);
    WiFi.mode(WIFI_AP);
    delay(50);

    bool apOk = false;
    if (cfg.ap_password.length() >= 8) {
        apOk = WiFi.softAP(cfg.ap_ssid.c_str(), cfg.ap_password.c_str());
    } else {
        apOk = WiFi.softAP(cfg.ap_ssid.c_str());
    }

    IPAddress actualIP = WiFi.softAPIP();
    Serial.printf("[Network] AP Started (%s): SSID '%s', IP: %s\n", 
                  apOk ? "OK" : "FAILED", cfg.ap_ssid.c_str(), actualIP.toString().c_str());

    // Start Captive Portal DNS on port 53 (redirect all queries to SoftAP IP)
    _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    bool dnsStarted = _dnsServer.start(53, "*", actualIP);
    Serial.printf("[Network] DNS Server on port 53 started: %s\n", dnsStarted ? "OK" : "FAILED");

    // Setup mDNS in AP mode too
    MDNS.end();
    if (MDNS.begin(cfg.hostname.c_str())) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[Network] mDNS responder started: http://%s.local\n", cfg.hostname.c_str());
    }

    _state = NET_AP_PORTAL;
    StatusLed::instance().setMode(LED_AP_PORTAL);
}

bool NetworkManager::isConnected() const {
    return _state == NET_CONNECTED_STA && WiFi.status() == WL_CONNECTED;
}

bool NetworkManager::isApMode() const {
    return _state == NET_AP_PORTAL;
}

NetworkState NetworkManager::getState() const {
    return _state;
}

String NetworkManager::getIpAddress() const {
    if (_state == NET_CONNECTED_STA) {
        return WiFi.localIP().toString();
    } else if (_state == NET_AP_PORTAL) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

int NetworkManager::getRssi() const {
    return WiFi.RSSI();
}

String NetworkManager::getSsid() const {
    const auto& cfg = ConfigManager::instance().get();
    return (_state == NET_CONNECTED_STA) ? cfg.wifi_ssid : cfg.ap_ssid;
}

void NetworkManager::tick() {
    uint32_t now = millis();

    if (_state == NET_AP_PORTAL) {
        for (int i = 0; i < 5; i++) {
            _dnsServer.processNextRequest();
        }
        return;
    }

    if (_state == NET_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            _state = NET_CONNECTED_STA;
            const auto& cfg = ConfigManager::instance().get();
            Serial.println("\n=============================================================");
            Serial.printf("  [+] Wi-Fi Connected Successfully to '%s'!\n", cfg.wifi_ssid.c_str());
            Serial.printf("  [-] Local IP:     %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("  [-] Web UI URL:   http://%s/ (or http://%s.local)\n",
                          WiFi.localIP().toString().c_str(), cfg.hostname.c_str());
            Serial.printf("  [-] Signal RSSI:  %d dBm\n", WiFi.RSSI());
            Serial.println("=============================================================\n");
            Serial.print("ESP32> ");

            MDNS.end();
            if (MDNS.begin(cfg.hostname.c_str())) {
                MDNS.addService("http", "tcp", 80);
            }

            StatusLed::instance().setMode(LED_POWER_OK);
        } else if (now - _connectStartMs > CONNECT_TIMEOUT_MS) {
            Serial.println("\n[Network] Connection timeout, falling back to AP Mode...");
            startApMode();
        }
        return;
    }

    if (_state == NET_CONNECTED_STA) {
        if (WiFi.status() != WL_CONNECTED) {
            if (now - _lastReconnectAttemptMs > 30000) {
                _lastReconnectAttemptMs = now;
                Serial.println("[Network] Connection lost, attempting reconnect...");
                WiFi.reconnect();
            }
        }
    }
}
