// wifi_manager.h
#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WifiManager
{
public:
    void begin(const char *ssid, const char *pass);
    void loop();
    bool connected() const;

private:
    const char *ssid = nullptr;
    const char *pass = nullptr;
    uint32_t lastTryMs = 0;
    uint32_t lastCheckMs = 0;
    bool isConnected = false;
};
