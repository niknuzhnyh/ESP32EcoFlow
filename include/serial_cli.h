#pragma once

#include <Arduino.h>

class SerialCli {
public:
    static SerialCli& instance();

    void begin();
    void tick();
    void printBanner();

private:
    SerialCli() = default;

    void processCommand(const String& cmdLine);
    void printHelp();
    void printStatus();
    void scanWifi();
    void connectWifi(const String& args);

    String _inputBuffer;
};
