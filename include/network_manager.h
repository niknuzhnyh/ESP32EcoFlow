#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>

enum NetworkState {
    NET_DISCONNECTED,
    NET_CONNECTING,
    NET_CONNECTED_STA,
    NET_AP_PORTAL
};

class NetworkManager {
public:
    static NetworkManager& instance();

    void begin();
    void tick();

    bool isConnected() const;
    bool isApMode() const;
    NetworkState getState() const;
    String getIpAddress() const;
    int getRssi() const;
    String getSsid() const;

    void startApMode();

private:
    NetworkManager() = default;

    NetworkState _state = NET_DISCONNECTED;
    DNSServer _dnsServer;
    uint32_t _connectStartMs = 0;
    uint32_t _lastReconnectAttemptMs = 0;
    const uint32_t CONNECT_TIMEOUT_MS = 20000;
};
