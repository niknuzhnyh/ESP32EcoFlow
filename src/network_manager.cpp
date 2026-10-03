#include "network_manager.h"
#include "config_manager.h"
#include "status_led.h"

NetworkManager& NetworkManager::instance() {
    static NetworkManager inst;
    return inst;
}

void NetworkManager::begin() {
    const auto& cfg = ConfigManager::instance().get();

    if (cfg.wifi_ssid.length() == 0) {
        Serial.println("[Network] No Wi-Fi SSID configured, launching AP mode");
        startApMode();
        return;
    }

    Serial.printf("[Network] Connecting to Wi-Fi SSID: %s\n", cfg.wifi_ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(cfg.hostname.c_str());
    WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_password.c_str());

    _state = NET_CONNECTING;
    _connectStartMs = millis();
    StatusLed::instance().setMode(LED_WIFI_CONNECTING);
}

void NetworkManager::startApMode() {
    const auto& cfg = ConfigManager::instance().get();
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);

    IPAddress apIP(192, 168, 4, 1);
    IPAddress netMsk(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, netMsk);

    if (cfg.ap_password.length() >= 8) {
        WiFi.softAP(cfg.ap_ssid.c_str(), cfg.ap_password.c_str(), 1, 0, 4);
    } else {
        WiFi.softAP(cfg.ap_ssid.c_str(), nullptr, 1, 0, 4);
    }

    Serial.printf("[Network] AP Started: SSID '%s', IP: %s\n", cfg.ap_ssid.c_str(), apIP.toString().c_str());

    // Start Captive Portal DNS on port 53 (redirect all queries to SoftAP IP)
    _dnsServer.stop();
    _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer.start(53, "*", apIP);

    // Setup mDNS in AP mode too
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
            Serial.printf("[Network] Connected! IP: %s, RSSI: %d dBm\n",
                          WiFi.localIP().toString().c_str(), WiFi.RSSI());

            if (MDNS.begin(cfg.hostname.c_str())) {
                MDNS.addService("http", "tcp", 80);
                Serial.printf("[Network] mDNS responder started: http://%s.local\n", cfg.hostname.c_str());
            }

            StatusLed::instance().setMode(LED_POWER_OK);
        } else if (now - _connectStartMs > CONNECT_TIMEOUT_MS) {
            Serial.println("[Network] Connection timeout, falling back to AP Mode...");
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
