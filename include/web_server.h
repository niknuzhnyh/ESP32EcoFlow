#pragma once

#include <Arduino.h>
#include <WebServer.h>

class WebServerManager {
public:
    static WebServerManager& instance();

    void begin();
    void tick();

private:
    WebServerManager() = default;

    void setupRoutes();
    void handleRoot();
    void handleStatus();
    void handleGetConfig();
    void handleSaveConfig();
    void handleTestAlert();
    void handleReboot();
    void handleFactoryReset();
    void handleCaptiveRedirect();
    void sendGzipHtml();
    void handleNotFound();

    WebServer _server{80};
    bool _rebootScheduled = false;
    uint32_t _rebootAtMs = 0;
};
